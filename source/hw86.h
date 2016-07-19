/*
 * Utilities for x86
 */

#ifndef _ARCHX86_H
#define _ARCHX86_H

/*
 * Functions defined in utilsx386.s
 */

 /*
  * Dump all regs, serial port
  */
 extern void dump_regs(void);
/*
 * Clears the screen for text mode 0
 */
extern void io_clear_screen(void);
/*
 * Send a character to the screen in teletype mode
 */
extern void io_out_char(char c);
/*
 * Send a character to the serial port
 */
extern void io_out_char_serial(char c);
/*
 * Send a character to the screen in specific position
 */
extern void io_put_char_attr(int x, int y, char c, char color);
/*
 * Hide the cursor
 */
extern void io_hide_cursor(void);
/*
 * Show the cursor
 */
extern void io_show_cursor(void);
/*
 * Set the cursor position
 */
extern void io_set_cursor_pos(int x, int y);
/*
 * Set the text mode to 80x25 with 16 colors
 */
extern void io_set_text_mode(void);
/*
 * Halt the computer
 */
extern void halt(void);
/*
 * Get keystroke
 */
extern unsigned int io_in_key(void);
/*
 * Get drive info
 */
int get_drive_info(int drive, int* st, int* hd, int* cl);
/*
 * Read disk sector
 */
extern int read_disk(int drive, int sector, int n, char* buff );
/*
 * Write disk sector
 */
extern int write_disk(int drive, int sector, int n, char* buff );
/*
 * Get system time
 */
extern void kernel_systime(char* BDCtime, char* date);
/*
 * Kernel system call
 */
extern void kernel_syscall(int service, void* param, int* result);

#endif   /* _ARCHX86_H */
