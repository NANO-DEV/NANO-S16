#include "types.h"
#include "hw86.h"
#include "kernel.h"
#include "video.h"
#include "ulib/ulib.h"

/*
 * Graphics mode special functions including
 * pixel drawing and terminal emulation
 */

/* Default colors */
#define DEF_BACKGROUND 0x00
#define DEF_TEXT       0x07

#define FNT_W 8 /* Font glyph width */
uint video_font_h = 8; /* Font glyph height (px). Width is assumed to be 8px */
uint video_window_y = 0; /* Start of screen window in memory */
/* video_window_y allows fast screen scrolling in terminal emulation mode */

/* Terminal emulation cursor */
static uint cursor_col = 0;
static uint cursor_row = 0;
static uint cursor_shown = 0;

/* Buffer for storing and restoring pixels behind emulated cursor */
static uchar cursor_buff[FNT_W-2];
static uint cursor_buff_x = 0;
static uint cursor_buff_y = 0;
static uint cursor_blink_state = 0; /* 0: hidden 1: shown */
ul_t cursor_blink_timer = 0;

/* Pointer to VESA VIDEO memory */
static lp_t VIDEO_MEM = 0x000A0000L;
#define VESA_BANK_SIZE  0x10000L /* 64KB memory granularity works in 95% computers */

/* Pointer to BIOS font and offset between chars */
static lp_t BIOS_font = 0;
static uint BIOS_font_offset = 8;

/* Current VESA bank */
static ul_t current_bank = 0;

/*
 * Enable video mode
 * Does not set mode
 */
static void update_cursor_buffer();
void video_enable()
{
  /* Reset window */
  video_window_y = 0;

  /* Reset cursor */
  cursor_col = 0;
  cursor_row = 0;
  cursor_shown = 1;

  /* Reset cursor buffer */
  cursor_buff_x = 0;
  cursor_buff_y = 0;
  cursor_blink_state = 0;
  update_cursor_buffer();

  /* Get font */
  if(BIOS_font == 0) {
    BIOS_font = io_get_bios_font(&BIOS_font_offset);
  }
}

/*
 * Disable video mode
 */
void video_disable()
{
  /* No longer a valid pointer */
  BIOS_font = 0;
}

/*
 * Get pixel
 */
static uint get_pixel(uint x, uint y, uint read_cursor_buffer)
{
  ul_t addr = (ul_t)x + (ul_t)screen_width_px*(ul_t)(y+video_window_y);
  ul_t bank_size = VESA_BANK_SIZE;
  ul_t bank_number = (ul_t)addr/(ul_t)bank_size;
  ul_t bank_offset = (ul_t)addr%(ul_t)bank_size;

  /* Read cursor pixel buffer */
  if(read_cursor_buffer != 0 && cursor_shown &&
    x>=cursor_buff_x && x<cursor_buff_x+FNT_W-2 &&
    y==cursor_buff_y) {
    return (uint)cursor_buff[x-cursor_buff_x];
  }

  /* This is very expensive, do only if actually needed */
  if(bank_number != current_bank) {
    io_set_vesa_bank(bank_number);
    current_bank = bank_number;
  }

  return (uint)lmem_getbyte(VIDEO_MEM + bank_offset);
}

/*
 * Set pixel
 */
void video_set_pixel(uint x, uint y, uint c)
{
  ul_t addr = (ul_t)x + (ul_t)screen_width_px*(ul_t)(y+video_window_y);
  ul_t bank_size = VESA_BANK_SIZE;
  ul_t bank_number = (ul_t)addr/(ul_t)bank_size;
  ul_t bank_offset = (ul_t)addr%(ul_t)bank_size;

  /* Update cursor pixel buffer */
  if(cursor_shown &&
    x>=cursor_buff_x && x<cursor_buff_x+FNT_W-2 &&
    y==cursor_buff_y) {
    cursor_buff[x-cursor_buff_x] = c;
  }

  /* This is very expensive, do only if actually needed */
  if(bank_number != current_bank) {
    io_set_vesa_bank(bank_number);
    current_bank = bank_number;
  }

  lmem_setbyte(VIDEO_MEM + bank_offset, c); /* Set */
}

/*
 * Set pixel, used to draw cursor, since
 * it does not update cursor pixel buffer
 */
static void set_cursor_pixel(uint x, uint y, uint c)
{
  ul_t addr = (ul_t)x + (ul_t)screen_width_px*(ul_t)(y+video_window_y);
  ul_t bank_size = VESA_BANK_SIZE;
  ul_t bank_number = (ul_t)addr/(ul_t)bank_size;
  ul_t bank_offset = (ul_t)addr%(ul_t)bank_size;

  /* This is very expensive, do only if actually needed */
  if(bank_number != current_bank) {
    io_set_vesa_bank(bank_number);
    current_bank = bank_number;
  }

  lmem_setbyte(VIDEO_MEM + bank_offset, c); /* Set */
}

/*
 * Get pixels from cursor screen area
 */
static void update_cursor_buffer()
{
  uint i;
  for(i=0; i<FNT_W-2; i++) {
    cursor_buff[i] =
      get_pixel(cursor_buff_x+i, cursor_buff_y, 0);
  }
}

/*
 * Draw pixels in cursor screen area
 */
static void draw_cursor_buffer()
{
  uint i;
  for(i=0; i<FNT_W-2; i++) {
    set_pixel(cursor_buff_x+i, cursor_buff_y,
      cursor_buff[i]);
  }
}

/*
 * Returns 0 if a char can be skipped (space, \n...)
 */
static uint is_visible_char(uchar c)
{
  if(c==' ' || c=='\r' || c=='\n' || c=='\t' || c==0) {
    return 0;
  }
  return 1;
}

/*
 * Clear the screen
 */
void video_clear_screen()
{
  uint i, j;
  uint tcursor_shown = cursor_shown;
  video_hide_cursor();
  video_window_y = 0; /* Reset window */

  /* Repaint full screen */
  for(j=0; j<screen_height_px; j++) {
    for(i=0; i<screen_width_px; i++){
      video_set_pixel(i, j, DEF_BACKGROUND);
    }
  }

  /* Switch to video window 0 */
  video_window_y = -video_font_h;
  io_scroll_screen();

  /* Reset cursor position */
  video_set_cursor_pos(0, 0);
  if(tcursor_shown) {
    video_show_cursor();
  }
}

/*
 * Get glyph from BIOS font
 */
static void get_BIOS_glyph(uchar* buff, uint character)
{
  uint i;
  lp_t char_addr = BIOS_font + (lp_t)BIOS_font_offset*(lp_t)character;
  for(i=0; i<video_font_h; i++) {
    buff[i] = lmem_getbyte(char_addr + (lp_t)i);
  }
}

/*
 * Draw a char at given x, y
 */
void video_draw_char(uint x, uint y, uint c, uint text_cl, uint back_cl)
{
  uint i, j;
  uchar buff[16]; /* This fixed size should be enough */

  if(is_visible_char(c)) {
    /* Get font glyph */
    get_BIOS_glyph(buff, c);

    /* Draw glyph */
    for(j=0; j<video_font_h; j++) {
      for(i=0; i<FNT_W; i++) {
        if(back_cl != NO_BACKGROUND) {
          uchar bit_color = (buff[j] & (0x80>>i)) ? text_cl : back_cl;
          video_set_pixel(x+i, y+j, bit_color);
        } else if(buff[j] & (0x80>>i)) {
          video_set_pixel(x+i, y+j, text_cl);
        }
      }
    }

  } else if(back_cl != NO_BACKGROUND) {
    /* Draw a space */
    for(j=0; j<video_font_h; j++) {
      for(i=0; i<FNT_W; i++) {
        video_set_pixel(x+i, y+j, back_cl);
      }
    }
  }

  return;
}

/*
 * Draw emulated cursor at given x, y
 */
static void draw_cursor(uint x, uint y, uint c)
{
  uint i;

  /* Draw bar */
  for(i=x; i<x+FNT_W-2; i++) {
    set_cursor_pixel(i, j, c);
  }

  return;
}

/*
 * Show terminal emulation cursor
 */
void video_show_cursor()
{
  if(!cursor_shown || !cursor_blink_state) {
    update_cursor_buffer();
  }
  cursor_shown = 1;
}

/*
 * Hide terminal emulation cursor
 */
void video_hide_cursor()
{
  if(cursor_shown && cursor_blink_state) {
    draw_cursor_buffer();
  }
  cursor_shown = 0;
}

/*
 * Get terminal emulation cursor position
 */
void video_get_cursor_pos(uint* col, uint* row)
{
  *col = cursor_col;
  *row = cursor_row;
}

/*
 * Set terminal emulation cursor position
 */
void video_set_cursor_pos(uint col, uint row)
{
  if(col != cursor_col || row != cursor_row) {
    if(cursor_shown) {
      draw_cursor_buffer();
    }
    cursor_col = col;
    cursor_row = row;
    cursor_buff_x = cursor_col * FNT_W + 1;
    cursor_buff_y = ((cursor_row+1)*video_font_h) - 1;
    if(cursor_shown) {
      update_cursor_buffer();
    }
  }
}

/*
 * Set terminal emulation cursor position, fast
 */
static void set_cursor_pos_fast(uint col, uint row)
{
  if(col != cursor_col || row != cursor_row) {
    cursor_col = col;
    cursor_row = row;
    cursor_buff_x = cursor_col * FNT_W + 1;
    cursor_buff_y = ((cursor_row+1)*video_font_h) - 1;
    if(cursor_shown) {
      update_cursor_buffer();
    }
  }
}

/*
 * Once a char has been drawn, update terminal emulation cursor
 */
static void update_cursor_after_char(uchar c)
{
  uint tc_row = cursor_row;
  uint tc_col = cursor_col;

  /* Special chars, move cursor */
  if(c == '\n') {
    tc_row++;
  } else if(c == '\r') {
    tc_col = 0;
  } else {
    tc_col++;
  }

  /* If new position exceeds line lenght, go to next line */
  if(tc_col > screen_width_c) {
    tc_col = 0;
    tc_row++;
  }

  /* If new position exceeds screen height, scroll */
  if(tc_row > screen_height_c-1) {
    uint tcursor_shown = cursor_shown;
    uint i, j;

    /* After two full screens, reset window */
    if(video_window_y > 2*screen_height_px) {
      uchar* scline = malloc(screen_width_px);
      uint v_w_y = video_window_y;
      video_window_y = 0;
      for(j=video_font_h; j<screen_height_px; j++) {
        for(i=0; i<screen_width_px; i++){
          scline[i] = get_pixel(i, j + v_w_y, 1);
        }
        for(i=0; i<screen_width_px; i++){
          video_set_pixel(i, j, (uint)scline[i]);
        }
      }
      mfree(scline);
    }

    /* Clear new lines */
    for(i=0; i<screen_width_px; i++){
      for(j=screen_height_px; j<screen_height_px+video_font_h; j++) {
        video_set_pixel(i, j, DEF_BACKGROUND);
      }
    }

    /* Fast hardware scroll without moving data */
    io_scroll_screen();

    /* Reset cursor */
    tc_col = 0;
    tc_row = screen_height_c-1;

  }

  set_cursor_pos_fast(tc_col, tc_row);
}

/*
 * Draw terminal emulation char in teletype mode
 */
void video_out_char(uchar c)
{
  video_draw_char(cursor_col*FNT_W, cursor_row*video_font_h, c, DEF_TEXT, DEF_BACKGROUND);
  update_cursor_after_char(c);
}

/*
 * Draw terminal emulation char with attributes
 */
void video_out_char_attr(uint col, uint row, uchar c, uchar attr)
{
  video_set_cursor_pos(col, row);
  video_draw_char(cursor_col*FNT_W, cursor_row*video_font_h, c, attr&0xF, attr>>4);
  update_cursor_after_char(c);
}

/*
 * Blink emulated cursor. Called by timer handler
 */
void video_blink_cursor()
{
  if(cursor_shown == 1) {
    cursor_blink_state = 1-cursor_blink_state;
    if(!cursor_blink_state) {
      draw_cursor_buffer();
    } else {
      draw_cursor(cursor_buff_x, cursor_buff_y, DEF_TEXT);
    }
  }
  cursor_blink_timer = system_timer_ms;
}
