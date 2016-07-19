/*
 * C-Lib
 */

#include "syscall.h"
#include "clib.h"

extern void clib_syscall(int service, void* param, int* result);

/*
 * Perform system call
 */
int syscall(int service, void* param)
{
    int result = 0;
    clib_syscall(service, param, &result);
    return result;
}

/*
 * Get HIGH byte
 */
char getHI(int c)
{
    return (c >> 8) & 0xFF;
}

/*
 * Get LOW byte
 */
char getLO(int c)
{
    return (c & 0xFF);
}

/*
 * Send a character to the serial port
 */
void sputchar(char c)
{
    syscall(SYSCALL_IO_OUT_CHAR_SERIAL, (void*)c);
}

/*
 * Send string to the serial port
 */
void sputs(char *str)
{
    while(*str) {
        sputchar(*(str++));
    }
}

/*
 * Send complex string to the serial port
 */
#define D_STR_SIZE 32
int sprintf(char *format, ...)
{
    int* args = (int*)(void*)(&format + 1);
    char digit[D_STR_SIZE];
    int  valO = 0;
    int  val = 0;
    int  valS = 0;
    int  n = 0;
    int  nS = 0;
    int  base = 10;
    int  d = 0;

    while(*format) {
        if(*format == '%') {
            format++;
            valO = *(args++);
            valS = (valO & 0x8000);
            val = (valO & 0x7FFF);
            base = 0;
            n = 0;
            nS = 0;
            digit[D_STR_SIZE-1] = 0;

            if(*format == 'd') {
                base = 10;
                nS = 0;
                if(valS) {
                    val = -valO;
                }
            } else if(*format == 'x') {
                base = 16;
                nS = 4;
                if(valS) {
                    valS = 8;
                }
            } else if(*format == 'b') {
                base = 2;
                n = 1;
                digit[D_STR_SIZE-2] = 'b';
                nS = 17;
                if(valS) {
                    valS = 1;
                }
            } else if(*format == 's') {
                sputs((char*)valO);
                format++;
            }

            if(base != 0) {
                do {
                    n++;
                    d = mod(val, base);
                    if(nS > 0 && n == nS) {
                        d += valS;
                    }
                    digit[D_STR_SIZE-1-n] = ( d <= 9 ? '0' + d : 'A' + d - 10 );
                    val = val / base;
                } while((val && n < D_STR_SIZE-1) || (n < nS));

                if(*format == 'x') {
                    n++;
                    digit[D_STR_SIZE-1-n] = 'x';
                    n++;
                    digit[D_STR_SIZE-1-n] = '0';
                } else if(*format == 'd' && valS) {
                    n++;
                    digit[D_STR_SIZE-1-n] = '-';
                }

                sputs(&(digit[D_STR_SIZE-1-n]));
                format++;
            }
        } else {
            sputchar(*(format++));
        }
    }

    return 0;
}

/*
 * Clears the screen
 */
void clear_screen(void)
{
    syscall(SYSCALL_IO_CLEAR_SCREEN, 0);
}

/*
 * Send a character to the screen
 */
void putchar(char c)
{
    syscall(SYSCALL_IO_OUT_CHAR, (void*)c);
}

/*
 * Display string on the screen
 */
void puts(char *str)
{
    while (*str) {
        putchar(*(str++));
    }
}

/*
 * Display complex string on the screen
 */
int printf(char *format, ...)
{
    int* args = (int*)(void*)(&format + 1);
    char digit[D_STR_SIZE];
    int  valO = 0;
    int  val = 0;
    int  valS = 0;
    int  n = 0;
    int  nS = 0;
    int  base = 10;
    int  d = 0;

    while(*format) {
        if(*format == '%') {
            format++;
            valO = *(args++);
            valS = (valO & 0x8000);
            val = (valO & 0x7FFF);
            base = 0;
            n = 0;
            nS = 0;
            digit[D_STR_SIZE-1] = 0;

            if(*format == 'd') {
                base = 10;
                nS = 0;
                if(valS) {
                    val = -valO;
                }
            } else if(*format == 'x') {
                base = 16;
                nS = 4;
                if(valS) {
                    valS = 8;
                }
            } else if(*format == 'b') {
                base = 2;
                n = 1;
                digit[D_STR_SIZE-2] = 'b';
                nS = 17;
                if(valS) {
                    valS = 1;
                }
            } else if(*format == 's') {
                puts((char*)valO);
                format++;
            }

            if(base != 0) {
                do {
                    n++;
                    d = mod(val, base);
                    if(nS > 0 && n == nS) {
                        d += valS;
                    }
                    digit[D_STR_SIZE-1-n] = ( d <= 9 ? '0' + d : 'A' + d - 10 );
                    val = val / base;
                } while((val && n < D_STR_SIZE-1) || (n < nS));

                if(*format == 'x') {
                    n++;
                    digit[D_STR_SIZE-1-n] = 'x';
                    n++;
                    digit[D_STR_SIZE-1-n] = '0';
                } else if(*format == 'd' && valS) {
                    n++;
                    digit[D_STR_SIZE-1-n] = '-';
                }

                puts(&(digit[D_STR_SIZE-1-n]));
                format++;
            }
        } else {
            putchar(*(format++));
        }
    }

    return 0;
}

/*
 * Send a character to the screen with attr
 */
void putchar_attr(int x, int y, char c, char attr)
{
    struct TSYSCALL_CHARATTR ca;
    ca.x = x;
    ca.y = y;
    ca.c = c;
    ca.attr = attr;
    syscall(SYSCALL_IO_PUT_CHAR_ATTR, &ca);
}

/*
 * Set cursor position
 */
void set_cursor_position(int x, int y)
{
    struct TSYSCALL_CHARATTR ca;
    ca.x = x;
    ca.y = y;
    ca.c = 0;
    ca.attr = 0;
    syscall(SYSCALL_IO_SET_CURSOR_POS, &ca);
}

/*
 * Get key press in a char
 */
char getchar(void)
{
    unsigned int c;
    syscall(SYSCALL_IO_IN_KEY, &c);
    return (char)(c & 0x00FF);
}

/*
 * Get key press
 */
int getkey(void)
{
    int c;
    syscall(SYSCALL_IO_IN_KEY, &c);
    return c;
}

/*
 * Get a string from user
 */
void gets(char* str)
{
    unsigned int i = 0;

    while(1) {
        str[i] = getchar();
        if(str[i] == KEY_LO_RETURN) {
            str[i] = 0;
            putchar('\n');
            putchar('\r');
            break;
        }
        if(str[i] == KEY_LO_BACKSPACE)    {
            str[i] = 0;
            if(i>0)    {
                i--;
                str[i] = 0;
                putchar(KEY_LO_BACKSPACE);
                putchar(0);
                putchar(KEY_LO_BACKSPACE);
            }
        } else if(str[i] < 32 || str[i] > 126) {
            str[i] = 0;
        } else {
            putchar(str[i]);
            i++;
        }
    }
    str[++i] = 0;
}

/*
 * Copy string src to dest
 */
int strcpy(char *dest, char *src)
{
    int i = 0;
    while(src[i] != 0) {
        dest[i] = src[i];
        i++;
    }
    dest[i] = 0;
    return i;
}

/*
 * Copy n elements from string src to dest
 */
int strncpy(char *dest, char *src, int n)
{
    int i = 0;
    while(src[i] != 0 && i < n) {
        dest[i] = src[i];
        i++;
    }
    dest[i] = 0;
    return i;
}

/*
 * Concatenate string src to dest
 */
int strcat(char *dest, char *src)
{
    int j = 0;
    int i = strlen(dest);
    while(src[j] != 0) {
        dest[i] = src[j];
        i++;
        j++;
    }
    dest[i] = 0;
    return i;
}

/*
 * Get string length
 */
int strlen(char* str)
{
    int i = 0;
    while(str[i] != 0) {
        i++;
    }
    return i;
}

/*
 * Compare strings
 */
int strcmp(char* str1, char* str2)
{
    int i = 0;
    while(str1[i] == str2[i] && str1[i] != 0) {
        i++;
    }
    return str1[i] - str2[i];
}

/*
 * Compare n elements of strings
 */
int strncmp(char* str1, char* str2, int n)
{
    int i = 0;
    while(str1[i] == str2[i] && str1[i] != 0 && i < n - 1) {
        i++;
    }
    return str1[i] - str2[i];
}

/*
 * Tokenize string
 */
int strtok(char* spl, int na, int ss, int*ns, char* str)
{
    int i = 0;
    int n = 0;
    int a = 0;
    int result = 0;

    /* Parse args */
    spl[0] = 0;
    while(str[i] != 0) {
        while(str[i] == ' ') {
            i++;
            if(n > 0 && a < na - 1) {
                spl[a*ss + n] = 0;
                a++;
                n = 0;
            }
        }
        if(n < ss - 1) {
            spl[a*ss + n] = str[i];
        }
        n++;
        i++;
    }
    spl[a*ss + n] = 0;
    *ns = a;

    return 0;
}

/*
 * Copy size bytes from src to dest
 */
int memcpy(char *dest, char *src, int size)
{
    int i = 0;
    for(i=0; i<size; i++) {
        dest[i] = src[i];
    }
    return i;
}

/*
 * Set size bytes from src to value
 */
int memset(char *dest, char value, int size)
{
    int i = 0;
    for(i=0; i<size; i++) {
        dest[i] = value;
    }
    return i;
}

/*
 * Allocate memory block
 */
void* malloc(int size)
{
    void* c;
    c = syscall(SYSCALL_MEM_ALLOCATE, &size);
    return c;
}

/*
 * Free memory block
 */
void free(void* ptr)
{
    syscall(SYSCALL_MEM_FREE, ptr);
}

/*
 * Get filesystem info
 */
int get_fsinfo(int* n, struct FS_INFO** info)
{
    int c;
    struct TSYSCALL_FSINFO fi;
    fi.n = n;
    fi.info = info;
    c = syscall(SYSCALL_FS_GET_INFO, &fi);
    return c;
}

/*
 * Get CWD
 */
int getCWD(struct FS_PATH* cwd)
{
    return syscall(SYSCALL_FS_GET_CWD, cwd);
}

/*
 * Set CWD
 */
int setCWD(struct FS_PATH* cwd)
{
    return syscall(SYSCALL_FS_SET_CWD, cwd);
}

/*
 * Get dir entry
 */
int get_direntry(char* dir, int n, struct FS_DIRENTRY* direntry)
{
    int c;
    struct TSYSCALL_DIRENTRY de;
    de.dir = dir;
    de.n = n;
    de.direntry = direntry;
    c = syscall(SYSCALL_FS_GET_DIR_ENTRY, &de);
    return c;
}

/*
 * Get entry
 */
int get_entry(char* file, struct FS_DIRENTRY* entry)
{
    int c;
    struct TSYSCALL_DIRENTRY de;
    de.dir = file;
    de.n = 0;
    de.direntry = entry;
    c = syscall(SYSCALL_FS_GET_ENTRY, &de);
    return c;
}

/*
 * Create Dir
 */
int create_dir(char* dir)
{
    int c;
    c = syscall(SYSCALL_FS_CREATE_DIR, dir);
    return c;
}

/*
 * Set file size
 */
int set_file_size(char* file, int size)
{
    int c;
    struct TSYSCALL_FILESIZE fs;
    fs.file = file;
    fs.size = size;
    c = syscall(SYSCALL_FS_SET_FILE_SIZE, fs);
    return c;
}

/*
 * Delete
 */
int delete(char* file)
{
    int c;
    c = syscall(SYSCALL_FS_DELETE, file);
    return c;
}

/*
 * Rename
 */
int rename(char* file, char* new_name)
{
    int c;
    struct TSYSCALL_RENAME rn;
    rn.file = file;
    rn.name = new_name;
    c = syscall(SYSCALL_FS_RENAME, rn);
    return c;
}

/*
 * Read file
 */
int read_file(char* file, int offset, int buff_size, char* buff)
{
    int c;
    struct TSYSCALL_FILEIO fio;
    fio.file = file;
    fio.offset = offset;
    fio.buff_size = buff_size;
    fio.buff = buff;
    c = syscall(SYSCALL_FS_READ_FILE, &fio);
    return c;
}

/*
 * Write file
 */
int write_file(char* file, int offset, int buff_size, char* buff)
{
    int c;
    struct TSYSCALL_FILEIO fio;
    fio.file = file;
    fio.offset = offset;
    fio.buff_size = buff_size;
    fio.buff = buff;
    c = syscall(SYSCALL_FS_WRITE_FILE, &fio);
    return c;
}

/*
 * Get system date and time
 */
void time(struct TIME* time)
{
    syscall(SYSCALL_CLK_GET_TIME, time);
}
