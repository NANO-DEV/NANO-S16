/*
 * Kernel
 */

#include "kernel.h"
#include "hw86.h"
#include "clib/clib.h"
#include "clib/syscall.h"
#include "filesystem/fs.h"

/*
 * Drive device related
 */
char device; /* Boot device */

struct KDRIVEINFO kdinfo[K_MAX_DRIVE];  /* Drive info */
int dsects;  /* Current drive sectors per track */
int dsides;  /* Current drive sides */

/*
 * Extern program call
 */
typedef int extern_main(char** a);

/*
 * Heap related
 */
#define KHEAP_MAX_BLOCK    0x0080U
#define KHEAP_MEM_SIZE     0x4000U
#define KHEAP_BLOCK_SIZE   (KHEAP_MEM_SIZE / KHEAP_MAX_BLOCK)

static char KHEAPADDR[KHEAP_MEM_SIZE];

struct KHEAPBLOCK {
    int     used;
    void*   ptr;
} kheap[KHEAP_MAX_BLOCK];

/*
 * Init heap
 */
static void kernel_heap_init()
{
    unsigned int i;
    for(i=0; i<KHEAP_MAX_BLOCK; i++) {
        kheap[i].used = 0;
        kheap[i].ptr = 0;
    }
}

/*
 * Allocate memory in heap
 */
static void* kernel_heap_alloc(int size)
{
    int n_alloc = 0;
    int n_found = 0;
    int i;
    int j;

    if(size == 0) {
        return 0;
    }

    n_alloc = size / KHEAP_BLOCK_SIZE +
        (mod(size, KHEAP_BLOCK_SIZE) ? 1 : 0);

    for(i=0; i<KHEAP_MAX_BLOCK; i++) {
        if(kheap[i].used) {
            n_found = 0;
        } else {
            n_found++;
            if(n_found >= n_alloc) {
                int bi = (i+1-n_alloc)*KHEAP_BLOCK_SIZE;
                void* addr = &KHEAPADDR[bi];
                for(j=i+1-n_alloc; j<=i; j++) {
                    kheap[j].ptr = addr;
                    kheap[j].used = 1;
                }

                return addr;
            }
        }
    }

    sprintf("DBG: Mem alloc: BAD ALLOC (%d bytes)\n\r", size);
    return 0;
}

/*
 * Free memory in heap
 */
static kernel_heap_free(void* ptr)
{
    int i;
    if(ptr != 0) {
        for(i=0; i<KHEAP_MAX_BLOCK; i++) {
            if(kheap[i].ptr == ptr && kheap[i].used) {
                kheap[i].used = 0;
                kheap[i].ptr = 0;
            }
        }
    }

    return;
}

/*
 * Handle system calls
 */
extern void kernel_service(int s, void* param, int* result)
{
    switch(s) {
    case SYSCALL_IO_CLEAR_SCREEN:
        io_clear_screen();
        *result = 0;
        break;
    case SYSCALL_IO_OUT_CHAR:
        io_out_char((char)param);
        *result = 0;
        break;
    case SYSCALL_IO_PUT_CHAR_ATTR: {
        struct TSYSCALL_CHARATTR* ca = param;
        io_put_char_attr(ca->x, ca->y, ca->c, ca->attr);
        *result = 0;
        break; }
    case SYSCALL_IO_SET_CURSOR_POS: {
        struct TSYSCALL_CHARATTR* ca = param;
        io_set_cursor_pos(ca->x, ca->y);
        *result = 0;
        break; }
    case SYSCALL_IO_OUT_CHAR_SERIAL:
        io_out_char_serial((char)param);
        *result = 0;
        break;
    case SYSCALL_IO_IN_KEY:
        *((int*)param) = io_in_key();
        *result = 0;
        break;
    case SYSCALL_MEM_ALLOCATE:
        *result = kernel_heap_alloc((int)*param);
        break;
    case SYSCALL_MEM_FREE:
        kernel_heap_free(param);
        *result = 0;
        break;
    case SYSCALL_FS_GET_INFO: {
        struct TSYSCALL_FSINFO* fi = param;
        *result = fs_get_info(fi->n, fi->info);
        break; }
    case SYSCALL_FS_GET_CWD:
        *result = fs_getCWD(param);
        break;
    case SYSCALL_FS_SET_CWD:
        *result = fs_setCWD(param);
        break;
    case SYSCALL_FS_GET_DIR_ENTRY: {
        struct TSYSCALL_DIRENTRY* de = param;
        *result = fs_get_direntry(de->dir, de->n, de->direntry);
        break; }
    case SYSCALL_FS_GET_ENTRY: {
        struct TSYSCALL_DIRENTRY* de = param;
        *result = fs_get_entry(de->dir, de->direntry);
        break; }
    case SYSCALL_FS_CREATE_DIR:
        *result = fs_create_dir(param);
        break;
    case SYSCALL_FS_SET_FILE_SIZE: {
        struct TSYSCALL_FILESIZE* fs = param;
        *result = fs_set_file_size(fs->file, fs->size);
        break; }
    case SYSCALL_FS_DELETE:
        *result = fs_delete(param);
        break;
    case SYSCALL_FS_RENAME: {
        struct TSYSCALL_RENAME* rn = param;
        *result = fs_rename(rn->file, rn->name);
        break; }
    case SYSCALL_FS_READ_FILE: {
        struct TSYSCALL_FILEIO* fio = param;
        *result = fs_read_file(fio->file, fio->offset, fio->buff_size, fio->buff);
        break; }
    case SYSCALL_FS_WRITE_FILE: {
        struct TSYSCALL_FILEIO* fio = param;
        *result = fs_write_file(fio->file, fio->offset, fio->buff_size, fio->buff);
        break; }
    case SYSCALL_CLK_GET_TIME: {
        char BCDtime[3];
        char BCDdate[3];
        kernel_systime(BCDtime, BCDdate);
        BCD_time_to_time(param, BCDtime, BCDdate);
        *result = 0;
        break; }
    }
}

/*
 * Set drive params
 */
extern void set_drive_params(int drive)
{
    drive = drive_to_index(drive);
    dsects = kdinfo[drive].sectors;
    dsides = kdinfo[drive].sides;
}

/*
 * The main kernel function
 */
#define CLI_MAX_ARG    5
#define CLI_ARG_SIZE  64
extern void kernel(void)
{
    int result;
    char str[CLI_MAX_ARG*CLI_ARG_SIZE];
    char arg[CLI_MAX_ARG][CLI_ARG_SIZE];
    struct FS_DIRENTRY direntry;
    struct FS_PATH cwd;
    int i;
    int n;
    int a;

    io_set_text_mode();
    io_show_cursor();
    io_clear_screen();

    /* Initialize drives */
    for(i=0; i<K_MAX_DRIVE; i++) {
        a = index_to_drive(i);
        result = get_drive_info(a, &kdinfo[i].sectors, &kdinfo[i].sides, &kdinfo[i].cylinders);
        if(result == 0) {
            sprintf("DRIVE (%x : sect_per_track=%d, sides=%d, cylinders=%d)\n\r", a,
                kdinfo[i].sectors, kdinfo[i].sides, kdinfo[i].cylinders);
        } else  {
            kdinfo[i].sectors = 0;
            kdinfo[i].sides = 0;
            kdinfo[i].cylinders = 0;
        }
    }

    kernel_heap_init();
    result = fs_init();

    if(result != 0) {
        printf("File System init failed (%d)\n\r", result);
        sprintf("File System init failed (%d)\n\r", result);
    }

    printf("Starting...\n\r");
    sprintf("Starting...\n\r");

    /* Integrated CLI */
    while(1) {
        memset(str, 0, CLI_MAX_ARG*CLI_ARG_SIZE);
        memset(arg, 0, CLI_MAX_ARG*CLI_ARG_SIZE);

        fs_getCWD(&cwd);
        printf("[%s]%s> ", drive_to_string(cwd.drive), cwd.path);
        gets(str);
        sprintf("[%x]%s> %s\n\r", cwd.drive, cwd.path, str);

        strtok(arg, CLI_MAX_ARG, CLI_ARG_SIZE, &a, str);

        if(arg[0][0] == 0) {
            /* Empty command line, skip */
        }
        /* Drive names */
        else if(string_is_drive(arg[0])) {
            if(a == 0) {
                strcpy(cwd.path, "/");
                cwd.drive = string_to_drive(arg[0]);
                result = fs_setCWD(&cwd);
                if(result != 0) {
                    printf("Error\n\r");
                }
            } else {
                printf("usage error\n\r");
            }
        }
        /* Built in commands */
        else if(strcmp(arg[0], "cls") == 0) {
            if(a == 0) {
                result = syscall(SYSCALL_IO_CLEAR_SCREEN, 0);
            } else {
                printf("usage error\n\r");
            }
        }
        else if(strcmp(arg[0], "cd") == 0) {
            if(a == 1) {
                strcpy(cwd.path, arg[1]);
                result = fs_setCWD(&cwd);
                if(result != 0) {
                    printf("error setting CWD\n\r");
                }
            } else {
                printf("usage error\n\r");
            }
        }
        else if(strcmp(arg[0], "dir") == 0) {
            if(a == 0) {
                n = fs_get_direntry(cwd.path, 0, 0);
                for(i=0; i<n; i++) {
                    fs_get_direntry(cwd.path, i, &direntry);

                    printf("%s   %s",
                       direntry.flags & FS_FLAG_DIR ? "dir " : "file",
                       direntry.name);

                    for(a=strlen(direntry.name); a<16; a++) {
                        printf(" ");
                    }

                    for(a=10000; a>=1; a=a/10) {
                        if(direntry.size > a) {
                            break;
                        }
                        printf(" ");
                    }

                    printf("%d bytes\n\r", direntry.size);
                }
            } else {
                printf("usage error\n\r");
            }
        }
        else if(strcmp(arg[0], "type") == 0) {
            if(a == 1) {
                result = fs_get_entry(arg[1], &direntry);
                if(result == 0 && (direntry.flags & FS_FLAG_FILE)) {
                    char buff_line[80];
                    n = 0;
                    i = 0;
                    result = 0;
                    while(i < direntry.size) {
                        int k = 0;
                        memset(buff_line, 0, 80);
                        result = fs_read_file(arg[1], i, min(direntry.size - i, 80), buff_line);
                        if(result != 0) {
                            printf("error reading file\n\r");
                            break;
                        }
                        a = 0;
                        while(buff_line[a] != 0 && a < 80) {
                            if(buff_line[a] == '\n') {
                                printf("\n\r");
                                n++;
                                if(n == 24) {
                                    printf("%d/%d Enter: Next page | Esc: Quit", i, direntry.size);
                                    while(k != KEY_LO_RETURN && k != KEY_LO_ESC) {
                                        k = getLO(getkey());
                                    }
                                    printf("\n\r");
                                    n = 0;
                                    i++;
                                    break;
                                }
                            } else {
                                putchar(buff_line[a]);
                            }
                            i++;
                            a++;
                        }
                        if(k == KEY_LO_ESC || buff_line[a] == 0) {
                            break;
                        }
                    }
                } else {
                    printf("error: can't find file '%s' or is not a file\n\r", arg[1]);
                }
            } else {
                printf("usage error\n\r");
            }
        }
        else if(strcmp(arg[0], "ren") == 0) {
            if(a == 2) {
                result = fs_rename(arg[1], arg[2]);
                if(result != 0) {
                    printf("error\n\r");
                }
            } else {
                printf("usage error\n\r");
            }
        }
        else if(strcmp(arg[0], "del") == 0) {
            if(a == 1) {
                result = fs_delete(arg[1]);
                if(result != 0) {
                    printf("error\n\r");
                }
            } else {
                printf("usage error\n\r");
            }
        }
        else if(strcmp(arg[0], "md") == 0) {
            if(a == 1) {
                result = fs_create_dir(arg[1]);
                if(result != 0) {
                    printf("error\n\r");
                }
            } else {
                printf("usage error\n\r");
            }
        }
        else if(strcmp(arg[0], "copy") == 0 || strcmp(arg[0], "xcopy") == 0) {
            if(strcmp(arg[0], "copy")  == 0 && a == 2 ||
               strcmp(arg[0], "xcopy") == 0 && a == 4) {
                int srcArg = 1;
                int dstArg = 2;
                char* buffer = 0;
                struct FS_DIRENTRY direntry;

                if(a == 4) {
                    srcArg = 2;
                    dstArg = 4;
                    fs_set_drive(string_to_drive(arg[1]));
                }

                result = fs_get_entry(arg[srcArg], &direntry);
                if(result == 0 && (direntry.flags & FS_FLAG_FILE) == 0 ) {
                    result = 1;
                }

                if(result == 0) {
                    buffer = malloc(direntry.size);
                    if(buffer != 0) {
                        result = read_file(arg[srcArg], 0, direntry.size, buffer);
                        if(result != 0) {
                            printf("Can't load source file (%s)\n\r", arg[srcArg]);
                        }
                    } else {
                        printf("Not enough memory\n\r");
                    }
                }

                if(result != 0) {
                    printf("Can't find '%s' or is not a file\n\r", arg[srcArg]);
                } else {
                    if(a == 4) {
                        srcArg = 2;
                        dstArg = 4;
                        fs_set_drive(string_to_drive(arg[3]));
                    }
                    fs_delete(arg[dstArg]);
                    result = write_file(arg[dstArg], 0, direntry.size, buffer);
                    if(result != 0) {
                        printf("Error writting file\n\r");
                    }

                    if(a == 4) {
                        fs_set_drive(device);
                    }

                    free(buffer);
                }
            } else {
                printf("usage error\n\r");
            }
        }
        else if(strcmp(arg[0], "sys") == 0) {
            if(a == 1) {
                int dest = string_to_drive(arg[1]);
                result = 0;
                if(dest != device) {
                    result = fs_sys_drive(dest, device);
                } else {
                    result = 0;
                    printf("Invalid dest drive. Try 'hd' or 'fd'\n\r");
                }
                if(result != 0) {
                    printf("error\n\r");
                } else {
                    printf("Sys completed\n\r");
                }
            } else {
                printf("usage error\n\r");
            }
        }
        else if(strcmp(arg[0], "time") == 0) {
            if(a == 0) {
                struct TIME ctime;
                time(&ctime);

                printf("20%d/%s%d/%s%d %s%d:%s%d:%s%d\n\r",
                        ctime.year,
                        ctime.month <10?"0":"", ctime.month,
                        ctime.day   <10?"0":"", ctime.day,
                        ctime.hour  <10?"0":"", ctime.hour,
                        ctime.minute<10?"0":"", ctime.minute,
                        ctime.second<10?"0":"", ctime.second);
            } else {
                printf("usage error\n\r");
            }
        }
        else if(strcmp(arg[0], "help") == 0) {
            if(a == 0) {
                printf("System version is 1.0\n\r\n\r");
                printf("Built-in commands\n\r");
                printf("cd <dir>        - change current dir\n\r");
                printf("cls             - clear the screen\n\r");
                printf("copy <s d>      - copy s file to d\n\r");
                printf("del <file>      - delete file or dir\n\r");
                printf("dir             - show current dir contents\n\r");
                printf("md  <name>      - create new dir\n\r");
                printf("ren <f n>       - rename f to n\n\r");
                printf("sys <drive>     - create system drive\n\r");
                printf("time            - show time and date\n\r");
                printf("type <file>     - show file contents\n\r");
                printf("xcopy <a s b d> - copy s in disk a to d in disk b\n\r");

                printf("\n\r");
                printf("To change current disk, type fd0|fd1|hd0|hd1\n\r");
            } else {
                printf("usage error\n\r");
            }
        }
        else {
            i = strlen(arg[0]);
            result = fs_get_entry(arg[0], &direntry);
            if(result == 0 && strcmp(&arg[0][i-4], ".bin") == 0 &&
               (direntry.flags & FS_FLAG_FILE)) {
                result = fs_read_file(arg[0], 0, min(direntry.size, 0x2000), 0xD000);
                sprintf("Program found: %s, %d bytes\n\r", arg[0], direntry.size);
            }

            if(result != 0) {
                printf("unkown command\n\r");
            } else {
                extern_main* m = 0xD000;
                m(str);
            }
        }
    }

    halt();
}
