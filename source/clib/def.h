/*
 * Definitions
 */

#ifndef _DEF_H
#define _DEF_H

/*
 * Special key codes
 */
#define KEY_LO_BACKSPACE 0x08
#define KEY_LO_RETURN    0x0D
#define KEY_LO_ESC       0x1B
#define KEY_HI_DEL       0x53
#define KEY_HI_END       0x4F
#define KEY_HI_HOME      0x47
#define KEY_HI_INS       0x52
#define KEY_HI_PG_DN     0x51
#define KEY_HI_PG_UP     0x49
#define KEY_HI_PRT_SC    0x72
#define KEY_HI_TAB       0x0F

#define KEY_HI_UP        0x48
#define KEY_HI_LEFT      0x4B
#define KEY_HI_RIGHT     0x4D
#define KEY_HI_DOWN      0x50

#define KEY_HI_F1        0x3B
#define KEY_HI_F2        0x3C
#define KEY_HI_F3        0x3D
#define KEY_HI_F4        0x3E
#define KEY_HI_F5        0x3F
#define KEY_HI_F6        0x40
#define KEY_HI_F7        0x41
#define KEY_HI_F8        0x42
#define KEY_HI_F9        0x43
#define KEY_HI_F10       0x44
#define KEY_HI_F11       0x85
#define KEY_HI_F12       0x86

/*
 * Attribute flags for text and background
 */
#define AT_T_BLACK    0x00
#define AT_T_BLUE     0x01
#define AT_T_GREEN    0x02
#define AT_T_CYAN     0x03
#define AT_T_RED      0x04
#define AT_T_MAGENTA  0x05
#define AT_T_BROWN    0x06
#define AT_T_LGRAY    0x07
#define AT_T_DGRAY    0x08
#define AT_T_LBLUE    0x09
#define AT_T_LGREEN   0x0A
#define AT_T_LCYAN    0x0B
#define AT_T_LRED     0x0C
#define AT_T_LMAGENTA 0x0D
#define AT_T_YELLOW   0x0E
#define AT_T_WHITE    0x0F

#define AT_B_BLACK    0x00
#define AT_B_BLUE     0x10
#define AT_B_GREEN    0x20
#define AT_B_CYAN     0x30
#define AT_B_RED      0x40
#define AT_B_MAGENTA  0x50
#define AT_B_BROWN    0x60
#define AT_B_LGRAY    0x70
#define AT_B_DGRAY    0x80
#define AT_B_LBLUE    0x90
#define AT_B_LGREEN   0xA0
#define AT_B_LCYAN    0xB0
#define AT_B_LRED     0xC0
#define AT_B_LMAGENTA 0xD0
#define AT_B_YELLOW   0xE0
#define AT_B_WHITE    0xF0

/*
 * Some macros and basic defines
 */
#define NULL 0
#define min(a,b) (a<b?a:b)
#define max(a,b) (a>b?a:b)

/*
 * File system related
 */
#define FS_NAME_LEN 32
#define FS_MAX_PATH 256

#define FS_FLAG_FILE 0x0001
#define FS_FLAG_DIR  0x0002

struct FS_DIRENTRY {
    char    name[FS_NAME_LEN];
    int     flags;
    int     size;
};

struct FS_PATH {
    char    path[FS_MAX_PATH];
    int     drive;
};

#define FS_TYPE_UNKNOWN 0
#define FS_TYPE_EXT2    1

struct FS_INFO {
    char    name[4];
    int     id;
    int     fs_type;
    int     fs_size;
    int     ds_size;
};

/*
 * Time related
 */
struct TIME {
    int    year;
    int    month;
    int    day;
    int    hour;
    int    minute;
    int    second;
};

#endif /* _DEF_H */
