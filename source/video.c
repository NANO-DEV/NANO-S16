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

/* Pointer to VESA VIDEO memory */
static lp_t VIDEO_MEM = 0x000A0000L;

/* Pointer to BIOS font and offset between chars */
static lp_t BIOS_font = 0;
static uint BIOS_font_offset = 8;

/* Current VESA bank */
static ul_t current_bank = 0;

/*
 * Enable video mode
 * Does not set mode
 */
void video_enable()
{
  /* Reset window */
  video_window_y = 0;

  /* Reset cursor */
  cursor_col = 0;
  cursor_row = 0;
  cursor_shown = 1;

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
static uint get_pixel(uint x, uint y)
{
  ul_t addr = (ul_t)x + (ul_t)screen_width_px*(ul_t)(y+video_window_y);
  ul_t bank_size = 0x10000L; /* 64KB memory granularity works in 95% computers */
  ul_t bank_number = (ul_t)addr/(ul_t)bank_size;
  ul_t bank_offset = (ul_t)addr%(ul_t)bank_size;

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
  ul_t bank_size = 0x10000L; /* 64KB memory granularity works in 95% computers */
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
  cursor_col = 0;
  cursor_row = 0;
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
 * Show terminal emulation cursor
 */
void video_show_cursor()
{
  cursor_shown = 1;
}

/*
 * Hide terminal emulation cursor
 */
void video_hide_cursor()
{
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
  cursor_col = col;
  cursor_row = row;
}

/*
 * Once a char has been drawn, update terminal emulation cursor
 */
static void update_cursor_after_char(uchar c)
{
  /* Special chars, move cursor */
  if(c == '\n') {
    cursor_row++;
  } else if(c == '\r') {
    cursor_col = 0;
  } else {
    cursor_col++;
  }

  /* If new position exceeds line lenght, go to next line */
  if(cursor_col > screen_width_c) {
    cursor_col = 0;
    cursor_row++;
  }

  /* If new positin exceeds screen height, scroll */
  if(cursor_row > screen_height_c-1) {
    uint i, j;

    /* After two full screens, reset window */
    if(video_window_y > 2*screen_height_px) {
      uchar* scline = malloc(screen_width_px);
      uint v_w_y = video_window_y;
      video_window_y = 0;
      for(j=video_font_h; j<screen_height_px; j++) {
        for(i=0; i<screen_width_px; i++){
          scline[i] = get_pixel(i, j + v_w_y);
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
    cursor_col = 0;
    cursor_row = screen_height_c-1;
  }
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
  cursor_col = col;
  cursor_row = row;
  video_draw_char(cursor_col*FNT_W, cursor_row*video_font_h, c, attr&0xF, attr>>4);
  update_cursor_after_char(c);
}
