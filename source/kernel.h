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
};

extern struct DISKINFO disk_info[MAX_DISK]; /* Disks info */
extern uchar system_disk;  /* System disk */
extern uchar serial_status;  /* Serial port status */

extern uint screen_width;
extern uint screen_height;

#endif   /* _KERNEL_H */
