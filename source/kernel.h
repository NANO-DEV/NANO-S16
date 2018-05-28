/*
 * Kernel
 */

#ifndef _KERNEL_H
#define _KERNEL_H

#define OS_VERSION_HI 2
#define OS_VERSION_LO 0
#define OS_BUILD_NUM 22

/*
 * Hardware related disk information is handled by the kernel module.
 * File system related information is handled by file system module
 */
#define MAX_DISK 4

/* Size of a disk sector */
#define SECTOR_SIZE 512

/* We need this buffer to be inside a 64KB bound
 * to avoid DMA error */
extern uchar disk_buff[SECTOR_SIZE];

struct DISKINFO {
    uint  id;          /* Disk id */
    uchar name[4];     /* Disk name */
    uint  fstype;      /* File system type: see ulib.h */
    ul_t  fssize;      /* Number of blocks in file system */
    uint  sectors;
    uint  sides;
    uint  cylinders;
    ul_t  size;        /* Disk size (MB) */
    ul_t  last_access; /* Last accessed time (system ms) */
} disk_info[MAX_DISK];

extern uchar system_disk; /* System disk */
extern uchar serial_status; /* Serial port status */
extern uchar a20_enabled; /* A20 line enabled */
extern uint serial_debug; /* Debug info through serial port */

extern uint graphics_mode; /* Graphics mode enabled */
extern uint screen_width_px; /* Screen width (pixels) */
extern uint screen_height_px; /* Screen height (pixels) */
extern uint screen_width_c; /* Screen width (text) */
extern uint screen_height_c; /* Screen height (text) */

extern ul_t system_timer_freq; /* Actual frequency of timer */
extern ul_t system_timer_ms; /* Number of whole ms since timer initialized */

#endif   /* _KERNEL_H */
