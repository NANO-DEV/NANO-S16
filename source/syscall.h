/*
 * Syscall
 */

#ifndef _SYSCALL_H
#define _SYSCALL_H

/*
 * System call service codes
 */
#define SYSCALL_IO_CLEAR_SCREEN         0x0000
#define SYSCALL_IO_OUT_CHAR             0x0010
#define SYSCALL_IO_OUT_CHAR_ATTR        0x0011
#define SYSCALL_IO_OUT_CHAR_SERIAL      0x0018
#define SYSCALL_IO_GET_CURSOR_POS       0x0020
#define SYSCALL_IO_SET_CURSOR_POS       0x0021
#define SYSCALL_IO_IN_KEY               0x0030
#define SYSCALL_MEM_ALLOCATE            0x0040
#define SYSCALL_MEM_FREE                0x0041
#define SYSCALL_FS_GET_INFO             0x0050
#define SYSCALL_FS_GET_ENTRY            0x0051
#define SYSCALL_FS_READ_FILE            0x0052
#define SYSCALL_FS_WRITE_FILE           0x0053
#define SYSCALL_FS_MOVE                 0x0054
#define SYSCALL_FS_COPY                 0x0055
#define SYSCALL_FS_DELETE               0x0056
#define SYSCALL_FS_CREATE_DIRECTORY     0x0057
#define SYSCALL_FS_LIST                 0x0058
#define SYSCALL_FS_FORMAT               0x0059
#define SYSCALL_CLK_GET_TIME            0x0070

/*
 * Syscall param structs
 *
 * System calls accept two parameters:
 * -Service code
 * -Raw pointer
 *
 * When a service requires more than one parameter,
 * these are packed in a TSYSCALL_ struct
 * and this struct is passed as the raw pointer param
 */

struct TSYSCALL_FSINFO {
  uint                disk_index;
  struct CFS_INFO*    info;
};

struct TSYSCALL_FSENTRY {
  struct FS_ENTRY*    entry;
  uchar*              path;
  uint                parent;
  uint                disk;
};

struct TSYSCALL_FSRWFILE {
  uchar*               buff;
  uchar*               path;
  uint                 offset;
  uint                 count;
  uint                 flags;
};

struct TSYSCALL_FSSRCDST {
  uchar*               src;
  uchar*               dst;
};

struct TSYSCALL_FSLIST {
  struct FS_ENTRY*     entry;
  uchar*               path;
  uint                 n;
};

struct TSYSCALL_CHARATTR {
  uint                 x;
  uint                 y;
  uchar                c;
  uchar                attr;
};

struct TSYSCALL_POSITION {
  uint                 x;
  uint                 y;
  uint*                px;
  uint*                py;
};

#endif   /* _SYSCALL_H */
