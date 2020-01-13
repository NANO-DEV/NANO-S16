/*
 * User program: Text editor
 */

#include "types.h"
#include "ulib/ulib.h"

/* modes for next_line function. See function */
#define SHOW_CURRENT 0
#define SKIP_CURRENT 1


/* char attributes for title bar and editor area */
/* title: black text over light gray background */
#define TITLE_ATTRIBUTES (AT_T_BLACK|AT_B_LGRAY)
/* editor: white text over blue background */
#define EDITOR_ATTRIBUTES (AT_T_WHITE|AT_B_BLUE)

/* Screen size */
static uint SCREEN_WIDTH = 80;
static uint SCREEN_HEIGHT = 25;

/* Mouse position and buttons */
static uint mouse_x = 0;
static uint mouse_y = 0;
static uint mouse_buttons = 0;

/* This buffer holds a copy of screen chars.
 * So, screen is only updated when it's actually needed */
static lp_t screen_buff = 0;

/*
 * Get first char of a far memory string.
 * Far memory needs to be accessed thorugh its API
 */
static uchar getlc(lp_t ptr)
{
  uchar c = 0;
  lmemcpy(lp(&c), ptr, 1L);
  return c;
}

/*
 * Set a char of a far memory string at a given offset
 * Far memory needs to be accessed thorugh its API
 */
static void setlc(lp_t ptr, ul_t offset, uchar c)
{
  lmemcpy(ptr + offset, lp(&c), 1L);
}

/*
 * Check the local screen buffer before actually updating
 * the screen
 */
static void editor_putchar(uint col, uint row, uchar c)
{
  uchar buff_c = 0;
  ul_t screen_offset = col + (row-1)*SCREEN_WIDTH;
  lmemcpy(lp(&buff_c), screen_buff + screen_offset, 1L);

  if(col==mouse_x && row==mouse_y) { /* Draw mouse where it's located */
    c = '+';
  }

  if(c != buff_c) {
    lmemcpy(screen_buff + screen_offset, lp(&c), 1L);
    putchar_attr(col, row, c, EDITOR_ATTRIBUTES);
  }
}

/*
 * Given a string (ext_ptr line input parameter)
 * returns a pointer to next line or to end of string
 * if there are not more lines
 *
 * Initial line of input string can be:
 * - shown at given row if mode==SHOW_CURRENT
 * - just skipped if mode==SKIP_CURRENT
 */
static lp_t next_line(uint mode, uint row, lp_t line)
{
  uint col = 0;

  /* Process string until next line or end of string
   * and show text if needed
   */
  while(line && getlc(line) && getlc(line)!='\n' && col<SCREEN_WIDTH) {
    if(mode == SHOW_CURRENT) {
      editor_putchar(col, row, getlc(line));
    }
    col++;
    line++;
  }

  /* Clear the rest of this line with spaces */
  while(col<SCREEN_WIDTH && mode == SHOW_CURRENT) {
    editor_putchar(col++, row, ' ');
  }

  /* Advance until next line begins */
  if(line && getlc(line) == '\n') {
    line++;
  }

  return line;
}

/*
 * Shows buffer in editor area starting at a given
 * number of line of buffer.
 * It's recommended to hide cursor before calling this function
 */
void show_buffer_at_line(lp_t buff, uint n)
{
  uint l = 0;

  /* Skip buffer until required line number */
  while(l<n && getlc(buff)) {
    buff = next_line(SKIP_CURRENT, 0, buff);
    l++;
  }

  /* Show required buffer lines */
  for(l=1; l<SCREEN_HEIGHT; l++) {
    if(getlc(buff)) {
      buff = next_line(SHOW_CURRENT, l, buff);
    } else {
      next_line(SHOW_CURRENT, l, 0L);
    }
  }
}

/*
 * Convert linear buffer offset (uint offset)
 * to line and col (output params)
 */
void buffer_offset_to_linecol(lp_t buff, ul_t offset, uint* col, uint* line)
{
  *col = 0;
  *line = 0;

  for(; offset>0 && getlc(buff); offset--, buff++) {

    if(getlc(buff) == '\n') {
      (*line)++;
      *col = 0;
    } else {
      (*col)++;
      if(*col >= SCREEN_WIDTH) {
        (*line)++;
        *col = 0;
      }
    }

  }
}

/*
 * Converts line and col (input params) to
 * linear buffer offset (return value)
 */
ul_t linecol_to_buffer_offset(lp_t buff, uint col, uint line)
{
  ul_t offset = 0;
  uint c = 0;
  while(getlc(buff) && line>0) {
    if(getlc(buff) == '\n' || c>=SCREEN_WIDTH) {
      line--;
      c=0;
    }
    c++;
    offset++;
    buff++;
  }

  while(getlc(buff) && col>0) {
    if(getlc(buff) == '\n') {
      break;
    }
    col--;
    offset++;
    buff++;
  }

  return offset;
}

/*
 * Convert linear buffer offset (uint offset)
 * to file line
 * Only function which is not screen space
 */
uint buffer_offset_to_fileline(lp_t buff, uint offset)
{
  uint line = 0;

  for(; offset>0 && getlc(buff); offset--, buff++) {

    if(getlc(buff) == '\n') {
      line++;
    }

  }

  return line;
}

/*
 * Program entry point
 */
uint main(uint argc, uchar* argv[])
{
  uchar* title_info = "L:     F1:Save ESC:Exit"; /* const */
  uchar  line_ibcd[4]; /* To store line number digits */
  uint   ibcdt = 0;

  uint i = 0;
  uint n = 0;
  uint result = 0;

  /* buff is fixed size and allocated in far memory so it
   * can be big enough.
   * buff_size is the size in bytes actually used in buff
   * buff_cursor_offset is the linear offset of current
   * cursor position inside buff
   */
  lp_t buff = 0;
  ul_t buff_size = 0;
  ul_t buff_cursor_offset = 0;

  /* First line number to display in the editor area */
  uint current_line = 0;

  /* Var to get key presses */
  uint k = 0;

  fs_entry_t entry;

  /* Chck usage */
  if(argc != 2) {
    putstr("Usage: %s <file>\n\r\n\r", argv[0]);
    putstr("<file> can be:\n\r");
    putstr("-an existing file path: opens existing file to edit\n\r");
    putstr("-a new file path: opens empty editor. File will be created on save\n\r");
    putstr("\n\r");
    return 1;
  }

  /* Allocate fixed size text buffer */
  buff = lmalloc(0xFFFFL);
  if(buff == 0) {
    putstr("Error: can't allocate memory\n\r");
    return 1;
  }

  /* Find file */
  n = get_entry(&entry, argv[1], UNKNOWN_VALUE, UNKNOWN_VALUE);

  /* Load file or show error */
  if(n<ERROR_ANY && (entry.flags & FST_FILE)) {
    ul_t offset = 0;
    uchar cbuff[512];
    setlc(buff, entry.size, 0);
    buff_size = entry.size;
    while(result = read_file(cbuff, argv[1], (uint)offset, sizeof(cbuff))) {
      if(result >= ERROR_ANY) {
        lmfree(buff);
        putstr("Can't read file %s (error=%x)\n\r", argv[1], result);
        return 1;
      }
      lmemcpy(buff + offset, lp(cbuff), (ul_t)result);
      offset += result;
    }
    if(offset != entry.size) {
      lmfree(buff);
      putstr("Can't read file (readed %d bytes, expected %d)\n\r",
        (uint)offset, entry.size);
      return 1;
    }
    /* Buffer must finish with a 0 and */
    /* must fit at least this 0, so buff_size can't be 0 */
    if(buff_size == 0 || getlc(buff + buff_size-1L) != 0) {
      buff_size++;
    }
  }

  /* Create 1 byte buffer if this is a new file */
  /* This byte is for the final 0 */
  if(buff_size == 0) {
    buff_size = 1;
    lmemset(buff, 0, buff_size);
  }

  /* Get screen size */
  get_screen_size(SSM_CHARS, &SCREEN_WIDTH, &SCREEN_HEIGHT);

  /* Allocate screen buffer */
  screen_buff = lmalloc((ul_t)(SCREEN_WIDTH*(SCREEN_HEIGHT-1)));
  if(screen_buff == 0) {
    putstr("Error: can't allocate memory\n\r");
    lmfree(buff);
    return 1;
  }

  /* Clear screen buffer */
  lmemset(screen_buff, 0, (ul_t)(SCREEN_WIDTH*(SCREEN_HEIGHT-1)));

  /* Write title */
  for(i=0; i<strlen(argv[1]); i++) {
    putchar_attr(i, 0, argv[1][i], TITLE_ATTRIBUTES);
  }
  for(; i<SCREEN_WIDTH-strlen(title_info); i++) {
    putchar_attr(i, 0, ' ', TITLE_ATTRIBUTES);
  }
  for(; i<SCREEN_WIDTH; i++) {
    putchar_attr(i, 0,
      title_info[i+strlen(title_info)-SCREEN_WIDTH], TITLE_ATTRIBUTES);
  }

  /* Show buffer and set cursor at start */
  set_show_cursor(HIDE_CURSOR);
  show_buffer_at_line(buff, current_line);
  set_cursor_position(0, 1);
  set_show_cursor(SHOW_CURSOR);

  /* Main loop */
  while(k != KEY_ESC) {
    uint col=0, line=0;

    /* Getmouse state */
    get_mouse_state(SSM_CHARS, &mouse_x, &mouse_y, &mouse_buttons);

    /* Process buttons */
    if(mouse_buttons & MOUSE_LEFT_BUTTON) {
      buff_cursor_offset = linecol_to_buffer_offset(buff, mouse_x, mouse_y-1);
    }

    /* Get key press */
    k = getkey(KM_NO_WAIT);

    /* Process key actions */

    /* Keys to ignore */
    if((k>KEY_F1 && k<=KEY_F10) ||
      k==KEY_F11 || k==KEY_F12 ||
      k==KEY_PRT_SC || k==KEY_INS ||
      k==0) {
      continue;

    /* Key F1: Save */
    } else if(k == KEY_F1) {
      ul_t offset = 0;
      uchar cbuff[512];
      result = 0;
      while(offset<buff_size && result<ERROR_ANY) {
        ul_t to_copy = min(sizeof(cbuff), buff_size-offset);
        lmemcpy(lp(cbuff), buff + offset, to_copy);
        result = write_file(cbuff, argv[1], (uint)offset, (uint)to_copy, FWF_CREATE | FWF_TRUNCATE);
        offset += to_copy;
      }

      /* Update state indicator */
      if(result < ERROR_ANY) {
        putchar_attr(strlen(argv[1]), 0, ' ', TITLE_ATTRIBUTES);
      } else {
        putchar_attr(strlen(argv[1]), 0, '*', (TITLE_ATTRIBUTES&0xF0)|AT_T_RED);
      }
      /* This opperation takes some time, so clear keyboard buffer */
      getkey(KM_CLEAR_BUFFER);

    /* Cursor keys: Move cursor */
    } else if(k == KEY_UP) {
      buffer_offset_to_linecol(buff, buff_cursor_offset, &col, &line);
      if(line > 0) {
        line -= 1;
        buff_cursor_offset = linecol_to_buffer_offset(buff, col, line);
      }

    } else if(k == KEY_DOWN) {
      buffer_offset_to_linecol(buff, buff_cursor_offset, &col, &line);
      line += 1;
      buff_cursor_offset = linecol_to_buffer_offset(buff, col, line);

    } else if(k == KEY_LEFT) {
      if(buff_cursor_offset > 0) {
        buff_cursor_offset--;
      }

    } else if(k == KEY_RIGHT) {
      if(buff_cursor_offset < buff_size - 1) {
        buff_cursor_offset++;
      }

    /* HOME, END, PG_UP and PG_DN keys */
    } else if(k == KEY_HOME) {
      buffer_offset_to_linecol(buff, buff_cursor_offset, &col, &line);
      col = 0;
      buff_cursor_offset = linecol_to_buffer_offset(buff, col, line);

    } else if(k == KEY_END) {
      buffer_offset_to_linecol(buff, buff_cursor_offset, &col, &line);
      col = 0xFFFF;
      buff_cursor_offset = linecol_to_buffer_offset(buff, col, line);

    } else if(k == KEY_PG_DN) {
      buffer_offset_to_linecol(buff, buff_cursor_offset, &col, &line);
      line += SCREEN_HEIGHT-1;
      buff_cursor_offset = linecol_to_buffer_offset(buff, col, line);

    } else if(k == KEY_PG_UP) {
      buffer_offset_to_linecol(buff, buff_cursor_offset, &col, &line);
      line -= min(line, SCREEN_HEIGHT-1);
      buff_cursor_offset = linecol_to_buffer_offset(buff, col, line);


    /* Backspace key: delete char before cursor and move cursor there */
    } else if(k == KEY_BACKSPACE) {
      if(buff_cursor_offset > 0) {
        lmemcpy(buff+buff_cursor_offset-1L, buff+buff_cursor_offset, buff_size-buff_cursor_offset);
        buff_size--;
        putchar_attr(strlen(argv[1]), 0, '*', TITLE_ATTRIBUTES);
        buff_cursor_offset--;
      }

    /* Del key: delete char at cursor */
    } else if(k == KEY_DEL) {
      if(buff_cursor_offset < buff_size-1) {
        lmemcpy(buff+buff_cursor_offset, buff+buff_cursor_offset+1, buff_size-buff_cursor_offset-1);
        buff_size--;
        putchar_attr(strlen(argv[1]), 0, '*', TITLE_ATTRIBUTES);
      }

    /* Any other key but esc: insert char at cursor */
    } else if(k != KEY_ESC && k != 0) {

      if(k == KEY_RETURN) {
        k = '\n';
      }
      if(k == KEY_TAB) {
        k = '\t';
      }
      lmemcpy(buff+buff_cursor_offset+1, buff+buff_cursor_offset, buff_size-buff_cursor_offset);
      setlc(buff, buff_cursor_offset++, k);
      buff_size++;
      putchar_attr(strlen(argv[1]), 0, '*', TITLE_ATTRIBUTES);
    }

    /* Update cursor position and display */
    buffer_offset_to_linecol(buff, buff_cursor_offset, &col, &line);
    if(line < current_line) {
      current_line = line;
    } else if(line > current_line + SCREEN_HEIGHT - 2) {
      current_line = line - SCREEN_HEIGHT + 2;
    }

    /* Update line number in title */
    /* Compute bcd value (reversed) */
    ibcdt = min(9999, buffer_offset_to_fileline(buff, buff_cursor_offset)+1);
    n = SCREEN_WIDTH-strlen(title_info)+2;
    for(i=0; i<4; i++) {
      line_ibcd[i] = ibcdt%10;
      ibcdt /= 10;
      if(ibcdt==0) {
        ibcdt = i;
        break;
      }
    }
    /* Display it */
    for(i=0; i<4; i++) {
      uchar c = i<=ibcdt?line_ibcd[ibcdt-i]+'0':' ';
      putchar_attr(n+i, 0, c, TITLE_ATTRIBUTES);
    }

    set_show_cursor(HIDE_CURSOR);
    show_buffer_at_line(buff, current_line);
    line -= current_line;
    line += 1;
    set_cursor_position(col, line);
    set_show_cursor(SHOW_CURSOR);
  }

  /* Free screen buffer */
  lmfree(screen_buff);

  /* Free buffer */
  lmfree(buff);

  /* Reset screen */
  clear_screen();
  set_cursor_position(0, 0);

  return 0;
}
