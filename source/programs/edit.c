/*
 * Text editor
 */

#include "../clib/clib.h"

#define DIR_NONE  0
#define DIR_LEFT  1
#define DIR_RIGHT 2
#define DIR_DOWN  3
#define DIR_UP    4

char* argv;
static char* filename = 0;
static char* buffer = 0;
static int buff_size = 0;
static int changed = 0;

static int cpos_x = 0;
static int cpos_y = 0;
static int scpos_x = 0;
static int scpos_y = 0;

/*
 * Get cursor position in buffer
 */
static void get_cursor_pos_in_buffer(int* x, int* y)
{
    *x = cpos_x + scpos_x;
    *y = cpos_y + scpos_y;
}

/*
 * Get buffer offset from position
 */
static int get_buffer_offset_from_pos(int cx, int cy)
{
    int i = 0;
    int x = 0;
    int y = 0;
    for(i=0; i<buff_size; i++) {
        if(y == cy) {
            for(x=0; x<cx; x++) {
                if(buffer[i+x] == 0 ||
                   buffer[i+x] == '\n') {
                    return i+x-1;
                }
            }
            return i+x;
        }
        if(buffer[i] == '\n') {
            y++;
            if(buffer[i+1] == '\r') {
                i++;
            }
        }
        if(buffer[i] == 0) {
            break;
        }
    }
    return buff_size;
}

/*
 * Get position from buffer offset
 */
static void get_pos_from_buffer_offset(int* x, int* y, int offset)
{
    int i = 0;
    *x = 0;
    *y = 0;
    for(i=0; i<buff_size; i++) {
        if(buffer[i] == '\n') {
            (*y)++;
            if(buffer[i+1] == '\r') {
                i++;
            }
            *x = 0;
        } else if(buffer[i] == 0 || i >= offset) {
            break;
        } else {
            (*x)++;
        }
    }
}

/*
 * Move cursor
 */
static void move_cursor(int* cx, int* cy, int* sx, int* sy, int dir)
{
    int bx = 0;
    int by = 0;
    int i = 0;
    int x = 0;
    int y = 0;

    if(dir == DIR_LEFT) {
        if(*cx > 0) {
            (*cx)--;
        } else if(*sx > 0) {
            (*sx)--;
        }
    } else if(dir == DIR_RIGHT) {
        if(*cx < 80) {
            (*cx)++;
        } else {
            (*sx)++;
        }
    } else if(dir == DIR_UP) {
        if(*cy > 0) {
            (*cy)--;
        } else if(*sy > 0) {
            (*sy)--;
        }
    } else if(dir == DIR_DOWN) {
        if(*cy < 23) {
            (*cy)++;
        } else {
            (*sy)++;
        }
    }

    bx = *cx + *sx;
    by = *cy + *sy;

    for(i=0; i<buff_size; i++) {
        if(buffer[i] == '\n') {
            y++;
            if(buffer[i+1] == '\r') {
                i++;
            }
        }

        if(y == by) {
            if(y > 0) {
                i++;
            }
            for(x=0; x<bx; x++) {
                if(buffer[i+x] == 0 ||
                buffer[i+x] == '\n') {
                    bx = x;
                    break;
                }
            }
            break;
        }
        if(buffer[i] == 0 || i == buff_size - 1) {
            by = y;
            i = -1;
            x = 0;
            y = 0;
        }
    }

    while(bx < *cx + *sx) {
        if(*cx > 0) {
            (*cx)--;
        } else if(*sx > 0) {
            (*sx)--;
        }
    }

    while(by < *cy + *sy) {
        if(*cy > 0) {
            (*cy)--;
        } else if(*sy > 0) {
            (*sy)--;
        }
    }
}

/*
 * Print line
 */
static void print_line(int cy)
{
    int i = get_buffer_offset_from_pos(0, cy);
    for(; i<buff_size; i++) {
        if(buffer[i] == 0 ||
           buffer[i] == '\n') {
            break;
        }
        putchar(buffer[i]);
    }
}

/*
 * Draw screen
 */
static void draw_screen()
{
    char title[80];
    int i = 0;
    int n = 0;

    int cx = 0;
    int cy = 0;
    get_cursor_pos_in_buffer(&cx, &cy);

    clear_screen();

    strcpy(title, filename);
    if(changed) {
        strcat(title, "* (F1:Save Esc:Exit)");
    } else {
        strcat(title, " (Esc:Exit)");
    }

    n = strlen(title);
    for(i=0; i<80; i++) {
        putchar_attr(i, 0, i<n?title[i]:0, AT_T_BLACK|AT_B_LGRAY);
    }

    for(i=scpos_y; i<scpos_y+24; i++) {
        printf("\n\r");
        print_line(i);
    }

    set_cursor_position(cpos_x, cpos_y+1);
}

/*
 * Insert at cursor
 */
static void insert_at_cursor(char c)
{
    int bx = 0;
    int by = 0;
    int bps = 0;
    int bpd = 0;
    int sz_inc = 1;
    char* buff_aux = 0;
    get_cursor_pos_in_buffer(&bx, &by);
    bps = get_buffer_offset_from_pos(bx, by);
    bpd = bps;

    switch(c) {
    case KEY_LO_RETURN:
        sz_inc = 2;
        break;
    case KEY_LO_BACKSPACE:
        if(bps > 0 && buffer[bps-1] == '\r') {
            buffer[bps-1] = ' ';
            sz_inc = -1;
            if(bps > 1 && buffer[bps-2] == '\n') {
                buffer[bps-2] = ' ';
                sz_inc = -2;
            }
        } else if(bps > 0) {
            sz_inc = -1;
        } else {
            sz_inc = 0;
        }

        get_pos_from_buffer_offset(&bx, &by, bps+sz_inc);
        cpos_x = bx - scpos_x;
        if(cpos_x < 0) {
            scpos_x += cpos_x;
            cpos_x = 0;
            if(scpos_x < 0) {
                scpos_x = 0;
            }
        }
        cpos_y = by - scpos_y;
        if(cpos_y < 0) {
            scpos_y += cpos_y;
            cpos_y = 0;
            if(scpos_y < 0) {
                scpos_y = 0;
            }
        }
        break;
    }

    buff_aux = malloc(buff_size+1+sz_inc);
    memcpy(buff_aux, buffer, bps);

    switch(c) {
    case KEY_LO_RETURN:
        buff_aux[bpd] = '\n';
        bpd++;
        buff_aux[bpd] = '\r';
        bpd++;
        memcpy(&buff_aux[bpd], &buffer[bps], buff_size + 1 - bps);
        break;
    case KEY_LO_BACKSPACE:
        bpd += sz_inc;
        memcpy(&buff_aux[bpd], &buffer[bps], buff_size + 1 - bps);
        break;
    default:
        buff_aux[bpd] = c;
        bpd++;
        memcpy(&buff_aux[bpd], &buffer[bps], buff_size + 1 - bps);
    }

    free(buffer);
    buffer = buff_aux;
    buff_size = buff_size + sz_inc;

    switch(c) {
    case KEY_LO_RETURN:
        cpos_x = 0;
        scpos_x = 0;
        move_cursor(&cpos_x, &cpos_y, &scpos_x, &scpos_y, DIR_DOWN);
        break;
    case KEY_LO_BACKSPACE:
        move_cursor(&cpos_x, &cpos_y, &scpos_x, &scpos_y, DIR_NONE);
        break;
    default:
        move_cursor(&cpos_x, &cpos_y, &scpos_x, &scpos_y, DIR_RIGHT);
    }

    changed = 1;
}

/*
 * Process user input
 */
static int process_input()
{
    int k  = getkey();
    int kh = getHI(k);
    int kl = getLO(k);

    if(kh == KEY_HI_DEL) {
        kl = KEY_LO_BACKSPACE;
    }

    if(kl == KEY_LO_ESC) {
        return 1;
    } else if(kh == KEY_HI_F1) {
        write_file(filename, 0, buff_size - 1, buffer);
        changed = 0;
    } else if(kh == KEY_HI_LEFT) {
        move_cursor(&cpos_x, &cpos_y, &scpos_x, &scpos_y, DIR_LEFT);
    } else if(kh == KEY_HI_RIGHT) {
        move_cursor(&cpos_x, &cpos_y, &scpos_x, &scpos_y, DIR_RIGHT);
    } else if(kh == KEY_HI_UP) {
        move_cursor(&cpos_x, &cpos_y, &scpos_x, &scpos_y, DIR_UP);
    } else if(kh == KEY_HI_DOWN) {
        move_cursor(&cpos_x, &cpos_y, &scpos_x, &scpos_y, DIR_DOWN);
    } else {
        insert_at_cursor(kl);
    }
    return 0;
}

/*
 * Entry point
 */
#define MAX_ARG   4
#define ARG_SIZE 64
int entry(void)
{
    char arg[MAX_ARG][ARG_SIZE];
    int i = 0;
    int n = 0;
    int a = 0;

    /* Parse args */
    strtok(arg, MAX_ARG, ARG_SIZE, &a, argv);

    if(a != 1) {
        printf("Usage: edit <file>\n\r");
        return 1;
    } else {
        /* Find file */
        struct FS_DIRENTRY entry;
        int result = 0;
        filename = arg[1];
        n = get_entry(arg[1], &entry);

        if(n == 0 && entry.flags & FS_FLAG_FILE) {
            /* Load file */
            buffer = malloc(entry.size+1);
            if(buffer != 0) {
                buffer[entry.size] = 0;
                buff_size = entry.size;
                result = read_file(arg[1], 0, entry.size, buffer);
                if(result != 0) {
                    free(buffer);
                    printf("Can't read file %s (error=%d)\n\r", arg[1], result);
                    return 1;
                }
            } else {
                printf("Not enough memory\n\r");
                return 1;
            }
        }

        if(buffer == 0) {
            /* Create buffer, new file */
            buffer = malloc(2);
            memset(buffer, 0, 2);
            buff_size = 1;
            changed = 1;
        }

        /* Main loop */
        i = 0;
        while(i == 0) {
            draw_screen();
            i = process_input();
        }

        /* Free buffer */
        if(buffer) {
            free(buffer);
        }
    }
    clear_screen();

    return 0;
}
