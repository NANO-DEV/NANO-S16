/*
 * Video utilities
 */

#ifndef _VIDEO_H
#define _VIDEO_H

/*
 * Enable video mode, once graphics mode is set
 */
void video_enable();

/*
 * Disable video mode, before graphics mode is unset
 */
void video_disable();

/*
 * Clear the screen
 */
void video_clear_screen();

/*
 * Set pixel color
 */
void video_set_pixel(uint x, uint y, uint c);

/*
 * Draw a character
 */
#define NO_BACKGROUND 0xFFFF
void video_draw_char(uint x, uint y, uint c, uint text_cl, uint back_cl);

/* Terminal emulation. See IO functions */
void video_show_cursor();
void video_hide_cursor();
void video_get_cursor_pos(uint* col, uint* row);
void video_set_cursor_pos(uint col, uint row);
void video_out_char(uchar c);
void video_out_char_attr(uint col, uint row, uchar c, uchar attr);


#endif /* _VIDEO_H */
