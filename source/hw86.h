/*
 * Hardware utilities for x86
 */

#ifndef _HWx86_H
#define _HWx86_H

/*
 * Functions defined in hw86.s
 */

/*
* Dump all register values to serial port
*/
extern void dump_regs();
/*
 * Set the VGA text mode to 80x25 with 16 colors
 */
extern void io_set_text_mode();
/*
 * Clears the screen
 */
extern void io_clear_screen();
/*
 * Send a character to the screen in teletype mode
 */
extern void io_out_char(uchar c);
/*
 * Send a character to the screen at specific position
 * with attributes (text and background color)
 */
extern void io_out_char_attr(uint x, uint y, uchar c, uchar attr);
/*
 * Send a character to the serial port
 */
extern void io_out_char_serial(uchar c);
/*
 * Read a character from the serial port
 */
extern uchar io_in_char_serial();
/*
 * Hide the cursor
 */
extern void io_hide_cursor();
/*
 * Show the cursor
 */
extern void io_show_cursor();
/*
 * Get the cursor position
 */
extern void io_get_cursor_pos(uint* x, uint* y);
/*
 * Set the cursor position
 */
extern void io_set_cursor_pos(uint x, uint y);
/*
 * Get keystroke
 */
extern uint io_in_key();
/*
 * Halt the computer
 */
extern void halt();
/*
 * Get disk hardware info
 */
extern uint get_disk_info(uint disk, uint* st, uint* hd, uint* cl);
/*
 * Read disk sector
 */
extern uint read_disk_sector(uint disk, uint sector, uint n, uchar* buff);
/*
 * Write disk sector
 */
extern uint write_disk_sector(uint disk, uint sector, uint n, uchar* buff);
/*
 * Get system time
 */
extern void get_time(uchar* BDCtime, uchar* date);

#endif   /* _HWx86_H */
