/*
 * Syscall
 */

#ifndef _SYSCALL_H
#define _SYSCALL_H

/*
 * Syscall codes
 */
#define SYSCALL_IO_CLEAR_SCREEN         0x0000
#define SYSCALL_IO_OUT_CHAR             0x0010
#define SYSCALL_IO_PUT_CHAR_ATTR        0x0011
#define SYSCALL_IO_SET_CURSOR_POS       0x0018
#define SYSCALL_IO_OUT_CHAR_SERIAL      0x0020
#define SYSCALL_IO_IN_KEY               0x0030
#define SYSCALL_MEM_ALLOCATE            0x0040
#define SYSCALL_MEM_FREE                0x0041
#define SYSCALL_FS_GET_INFO             0x0050
#define SYSCALL_FS_GET_CWD              0x0051
#define SYSCALL_FS_SET_CWD              0x0052
#define SYSCALL_FS_GET_DIR_ENTRY        0x0053
#define SYSCALL_FS_GET_ENTRY            0x0054
#define SYSCALL_FS_CREATE_DIR           0x0055
#define SYSCALL_FS_SET_FILE_SIZE        0x0056
#define SYSCALL_FS_DELETE               0x0057
#define SYSCALL_FS_RENAME               0x0058
#define SYSCALL_FS_READ_FILE            0x0059
#define SYSCALL_FS_WRITE_FILE           0x005A
#define SYSCALL_CLK_GET_TIME            0x0070

/*
 * Syscall param structs
 */
struct TSYSCALL_FSINFO {
    int*                n;
    struct CFS_INFO**   info;
};

struct TSYSCALL_DIRENTRY {
    char*               dir;
    int                 n;
    struct FS_DIRENTRY* direntry;
};

struct TSYSCALL_FILESIZE {
    char*   file;
    int     size;
};

struct TSYSCALL_RENAME {
    char*   file;
    char*   name;
};

struct TSYSCALL_FILEIO {
    char*   file;
    int     offset;
    int     buff_size;
    char*   buff;
};

struct TSYSCALL_CHARATTR {
    int     x;
    int     y;
    char    c;
    char    attr;
};

#endif   /* _SYSCALL_H */
