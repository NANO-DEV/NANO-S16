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
 * Set the VGA text mode
 */
extern void io_set_text_mode();
/*
 * Set the graphics mode
 */
extern void io_set_graphics_mode();
/*
 * Set active VESA bank (only graphics mode)
 */
extern void io_set_vesa_bank(ul_t bank);
/*
 * Get BIOS font pointer and offset (only graphics mode)
 */
extern lp_t io_get_bios_font(uint* offset);
/*
 * Clears the screen (all modes)
 */
extern void io_clear_screen();
/*
 * Scrolls the screen one line (all modes)
 */
extern void io_scroll_screen();
/*
 * Send a character to the screen in teletype mode (all modes)
 */
extern void io_out_char(uchar c);
/*
 * Send a character to the screen at specific position
 * with attributes (text and background color) (all modes)
 */
extern void io_out_char_attr(uint col, uint row, uchar c, uchar attr);
/*
 * Hide the cursor (all modes)
 */
extern void io_hide_cursor();
/*
 * Show the cursor (all modes)
 */
extern void io_show_cursor();
/*
 * Get the cursor position (all modes)
 */
extern void io_get_cursor_pos(uint* col, uint* row);
/*
 * Set the cursor position (all modes)
 */
extern void io_set_cursor_pos(uint col, uint row);
/*
 * Get keystroke, returns 0 if no pressed keys in buffer
 */
extern uint io_in_key();
/*
 * Send a character to the serial port
 */
extern void io_out_char_serial(uchar c);
/*
 * Read a character from the serial port
 */
extern uchar io_in_char_serial();
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
 * Turn off floppy disk motors
 */
extern void turn_off_fd_motors();
/*
 * Get system time
 */
extern void get_time(uchar* BDCtime, uchar* date);
/*
 * Set far memory byte
 */
extern void lmem_setbyte(lp_t addr, uchar b);
/*
 * Get far memory byte
 */
extern uchar lmem_getbyte(lp_t addr);
/*
 * User program far call
 */
extern uint uprog_call(uint argc, uchar* argv[]);
/*
 * Write byte to port
 */
extern void outb(uchar value, uint port);
/*
 * Read byte from port
 */
extern uchar inb(uint port);
/*
 * Write word to port
 */
extern void outw(uint value, uint port);
/*
 * Read word from port
 */
extern uint inw(uint port);
/*
 * Power off system using APM
 */
extern void apm_shutdown();
/*
 * Reboot system
 */
extern void reboot();
/*
 * Halt the computer
 */
extern void halt();
/*
 * Initialize timer (PIT)
 */
void timer_init(ul_t freq);
/*
 * Initialize PIC
 */
void PIC_init();

#endif   /* _HWx86_H */
