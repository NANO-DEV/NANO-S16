/*
 * Kernel
 */

#ifndef _KERNEL_H
#define _KERNEL_H

/*
 * Defines and data types
 */
#define K_MAX_DRIVE 4

struct KDRIVEINFO {
    int fstype;
    int fssize;
    int sectors;
    int sides;
    int cylinders;
};

/*
 * Shared variables
 */
extern struct KDRIVEINFO kdinfo[K_MAX_DRIVE];
extern char device;   /* Boot drive device */

#endif   /* _KERNEL_H */
