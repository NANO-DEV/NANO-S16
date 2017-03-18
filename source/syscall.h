/*
 * Syscall
 */

#ifndef _SYSCALL_H
#define _SYSCALL_H

/*
 * System call service codes
 */
#define SYSCALL_IO_GET_VIDEO_MODE       0x0000
#define SYSCALL_IO_SET_VIDEO_MODE       0x0001
#define SYSCALL_IO_GET_SCREEN_SIZE      0x0002
#define SYSCALL_IO_CLEAR_SCREEN         0x0003
#define SYSCALL_IO_SET_PIXEL            0x0008
#define SYSCALL_IO_DRAW_CHAR            0x000A
#define SYSCALL_IO_OUT_CHAR             0x0010
#define SYSCALL_IO_OUT_CHAR_ATTR        0x0011
#define SYSCALL_IO_GET_CURSOR_POS       0x0018
#define SYSCALL_IO_SET_CURSOR_POS       0x0019
#define SYSCALL_IO_SET_SHOW_CURSOR      0x001A
#define SYSCALL_IO_IN_KEY               0x0020
#define SYSCALL_IO_GET_MOUSE_STATE      0x0028
#define SYSCALL_IO_IN_CHAR_SERIAL       0x0030
#define SYSCALL_IO_OUT_CHAR_SERIAL      0x0031
#define SYSCALL_IO_OUT_CHAR_DEBUG       0x0038
#define SYSCALL_MEM_ALLOCATE            0x0040
#define SYSCALL_MEM_FREE                0x0041
#define SYSCALL_LMEM_ALLOCATE           0x0048
#define SYSCALL_LMEM_FREE               0x0049
#define SYSCALL_LMEM_GET                0x004A
#define SYSCALL_LMEM_SET                0x004B
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
#define SYSCALL_CLK_GET_TIME            0x0060
#define SYSCALL_CLK_GET_MILISEC         0x0061
#define SYSCALL_NET_RECV                0x0070
#define SYSCALL_NET_SEND                0x0071

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
  uint               disk_index;
  lp_t               info; /* FS_INFO */
};

struct TSYSCALL_FSENTRY {
  lp_t               entry; /* FS_ENTRY */
  lp_t               path; /* str */
  uint               parent;
  uint               disk;
};

struct TSYSCALL_FSRWFILE {
  lp_t               buff; /* byte[] */
  lp_t               path; /* str */
  uint               offset;
  uint               count;
  uint               flags;
};

struct TSYSCALL_FSSRCDST {
  lp_t               src; /* str */
  lp_t               dst; /* str */
};

struct TSYSCALL_FSLIST {
  lp_t               entry; /* FS_ENTRY */
  lp_t               path; /* str */
  uint               n;
};

struct TSYSCALL_POSATTR {
  uint               x;
  uint               y;
  uint               c;
  uint               attr;
};

struct TSYSCALL_POSITION {
  uint               x;
  uint               y;
  lp_t               px; /* uint */
  lp_t               py; /* uint */
};

struct TSYSCALL_LMEM {
  lp_t               dst;
  ul_t               n;
};

struct TSYSCALL_NETOP {
  lp_t               addr; /* uint8_t[4] */
  lp_t               buff; /* byte[] */
  uint               size;
};

#endif   /* _SYSCALL_H */
