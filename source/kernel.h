/*
 * Kernel
 */

#ifndef _KERNEL_H
#define _KERNEL_H

/*
 * Hardware related disk information is handled by the kernel module.
 * File system related information is handled by file system module
 */
#define MAX_DISK 4

/* Size of a disk sector */
#define SECTOR_SIZE 512

struct DISKINFO {
    uint     id;          /* Disk id */
    uchar    name[4];     /* Disk name */
    uint     fstype;      /* File system type: see ulib.h */
    uint32_t fssize;      /* Number of blocks in file system */
    uint     sectors;
    uint     sides;
    uint     cylinders;
    uint32_t size;        /* Disk size (MB) */
    uint32_t last_access; /* Last accessed time (system ms) */
};

extern struct DISKINFO disk_info[MAX_DISK]; /* Disks info */
extern uchar system_disk;  /* System disk */
extern uchar serial_status;  /* Serial port status */
extern uchar a20_enabled; /* A20 line enabled */
extern uint serial_debug; /* Debug info through serial port */

extern uint screen_width; /* Screen width */
extern uint screen_height; /* Screen height */

extern uint32_t IRQ0_frequency; /* Actual frequency of timer */
extern uint32_t system_timer_ms; /* Number of whole ms since timer initialized */

#endif   /* _KERNEL_H */
