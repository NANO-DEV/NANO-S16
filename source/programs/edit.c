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

/*
 * Get first char of an extended memory string
 * Extended memory needs to be accessed thorugh its API
 */
static uchar getexc(ex_ptr ptr)
{
  uchar c = 0;
  exmemcpy((ex_ptr)&c, (uint32_t)0, ptr, (uint32_t)0, (uint32_t)1);
  return c;
}

/*
 * Set a char of an extended memory string at a given offset
 * Extended memory needs to be accessed thorugh its API
 */
static void setexc(ex_ptr ptr, uint32_t offset, uchar c)
{
  exmemcpy(ptr, offset, (ex_ptr)&c, (uint32_t)0, (uint32_t)1);
}

/*
 * Given a string (ext_ptr line input parameter)
 * returns a pointer to next line or to end of string
 * if there are not more lines
 *
 * Initial line of input string can be:
 * - shown at given screen_y if mode==SHOW_CURRENT
 * - just skipped if mode==SKIP_CURRENT
 */
static ex_ptr next_line(uint mode, uint screen_y, ex_ptr line)
{
  uint x = 0;

  /* Process string until next line or end of string
   * and show text if needed
   */
  while(line && getexc(line) && getexc(line)!='\n' && x<SCREEN_WIDTH) {
    if(mode == SHOW_CURRENT) {
      putchar_attr(x++, screen_y, getexc(line), EDITOR_ATTRIBUTES);
    }
    line++;
  }

  /* Clear the rest of this line with spaces */
  while(x<SCREEN_WIDTH && mode == SHOW_CURRENT) {
    putchar_attr(x++, screen_y, ' ', EDITOR_ATTRIBUTES);
  }

  /* Advance until next line begins */
  if(line && getexc(line) == '\n') {
    line++;
  }

  return line;
}

/*
 * Shows buffer in editor area starting at a given
 * number of line of buffer.
 * It's recommended to hide cursor before calling this function
 */
void show_buffer_at_line(ex_ptr buff, uint n)
{
  uint l = 0;

  /* Skip buffer until required line number */
  while(l<n && getexc(buff)) {
    buff = next_line(SKIP_CURRENT, 0, buff);
    l++;
  }

  /* Show required buffer lines */
  for(l=1; l<SCREEN_HEIGHT; l++) {
    if(getexc(buff)) {
      buff = next_line(SHOW_CURRENT, l, buff);
    } else {
      next_line(SHOW_CURRENT, l, (ex_ptr)0);
    }
  }
}

/*
 * Convert linear buffer offset (uint offset)
 * to line and col (output params)
 */
void buffer_offset_to_linecol(ex_ptr buff, uint32_t offset, uint* col, uint* line)
{
  *col = 0;
  *line = 0;

  for(; offset>0 && getexc(buff); offset--, buff++) {

    if(getexc(buff) == '\n') {
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
uint32_t linecol_to_buffer_offset(ex_ptr buff, uint col, uint line)
{
  uint32_t offset = 0;
  while(getexc(buff) && line>0) {
    if(getexc(buff) == '\n') {
      line--;
    }
    offset++;
    buff++;
  }

  while(getexc(buff) && col>0) {
    if(getexc(buff) == '\n') {
      break;
    }
    col--;
    offset++;
    buff++;
  }

  return offset;
}

/*
 * Program entry point
 */
uint main(uint argc, uchar* argv[])
{
  uchar* title_info = "F1:Save ESC:Exit"; /* const */

  uint   i = 0;
  uint   n = 0;
  uint   result = 0;

  /* buff is fixed size and allocated in extended memory
   * so it can be big enough.
   * buff_size is the size in bytes actually used in buff
   * buff_cursor_offset is the linear offset of current
   * cursor position inside buff
   */
  ex_ptr   buff = 0;
  uint32_t buff_size = 0;
  uint32_t buff_cursor_offset = 0;

  /* First line number to display in the editor area */
  uint   current_line = 0;

  /* Vars to get key presses */
  uint   k  = 0;
  uint   kh = 0;
  uint   kl = 0;

  struct FS_ENTRY entry;

  /* Chck usage */
  if(argc != 2) {
    putstr("Usage: %s <file>\n\r\n\r", argv[0]);
    putstr("<file> can be:\n\r");
    putstr("-an existing file path: opens existing file to edit\n\r");
    putstr("-a new file path: opens empty editor. File will be created on save\n\r");
    putstr("\n\r");
    return 1;
  }

  /* Allocate fixed size buffer */
  buff = exmalloc((uint32_t)0xFFFF);
  if(buff == 0) {
    putstr("Error: can't allocate memory\n\r");
    return 1;
  }

  /* Find file */
  n = get_entry(&entry, argv[1], UNKNOWN_VALUE, UNKNOWN_VALUE);

  /* Load file or show error */
  if(n<ERROR_ANY && (entry.flags & FST_FILE)) {
    uint32_t offset = 0;
    uchar cbuff[512];
    setexc(buff, entry.size, 0);
    buff_size = entry.size;
    while(result = read_file(cbuff, argv[1], (uint)offset, sizeof(cbuff))) {
      if(result >= ERROR_ANY) {
        exmfree(buff);
        putstr("Can't read file %s (error=%x)\n\r", argv[1], result);
        return 1;
      }
      exmemcpy(buff, offset, (ex_ptr)cbuff, (uint32_t)0, (uint32_t)result);
      offset += result;
    }
    if(offset != entry.size) {
      exmfree(buff);
      putstr("Can't read file (readed %d bytes, expected %d)\n\r",
        (uint)offset, entry.size);
      return 1;
    }
    /* Buffer must finish with a 0 and */
    /* must fit at least this 0, so buff_size can't be 0 */
    if(buff_size == 0 || getexc(buff + buff_size-(ex_ptr)1) != 0) {
      buff_size++;
    }
  }

  /* Create 1 byte buffer if this is a new file */
  /* This byte is for the final 0 */
  if(buff_size == 0) {
    buff_size = 1;
    exmemset(buff, 0, buff_size);
  }

  /* Get screen size */
  get_screen_size(&SCREEN_WIDTH, &SCREEN_HEIGHT);

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
  while(kl != KEY_LO_ESC) {
    uint col, line;

    /* Get key press */
    k  = getkey();
    kh = getHI(k);
    kl = getLO(k);

    /* Process key actions */

    /* Key F1: Save */
    if(kh == KEY_HI_F1) {
      uint32_t offset = 0;
      uchar cbuff[512];
      result = 0;
      while(offset<buff_size && result<ERROR_ANY) {
        uint32_t to_copy = min(sizeof(cbuff), buff_size-offset);
        exmemcpy((ex_ptr)cbuff, (uint32_t)0, buff, offset, to_copy);
        result = write_file(cbuff, argv[1], (uint)offset, (uint)to_copy, FWF_CREATE | FWF_TRUNCATE);
        offset += to_copy;
      }
      if(result < ERROR_ANY) {
        putchar_attr(strlen(argv[1]), 0, ' ', TITLE_ATTRIBUTES);
      } else {
        putchar_attr(strlen(argv[1]), 0, '*', (TITLE_ATTRIBUTES&0xF0)|AT_T_RED);
      }

    /* Cursor keys: Move cursor */
    } else if(kh == KEY_HI_UP) {
      buffer_offset_to_linecol(buff, buff_cursor_offset, &col, &line);
      if(line > 0) {
        line -= 1;
        buff_cursor_offset = linecol_to_buffer_offset(buff, col, line);
      }

    } else if(kh == KEY_HI_DOWN) {
      buffer_offset_to_linecol(buff, buff_cursor_offset, &col, &line);
      line += 1;
      buff_cursor_offset = linecol_to_buffer_offset(buff, col, line);

    } else if(kh == KEY_HI_LEFT) {
      if(buff_cursor_offset > 0) {
        buff_cursor_offset--;
      }

    } else if(kh == KEY_HI_RIGHT) {
      if(buff_cursor_offset < buff_size - 1) {
        buff_cursor_offset++;
      }


    /* Backspace key: delete char before cursor and move cursor there */
    } else if(kl == KEY_LO_BACKSPACE) {
      if(buff_cursor_offset > 0) {
        exmemcpy(buff, buff_cursor_offset-1, buff, buff_cursor_offset, buff_size-buff_cursor_offset);
        buff_size--;
        putchar_attr(strlen(argv[1]), 0, '*', TITLE_ATTRIBUTES);
        buff_cursor_offset--;
      }

    /* Del key: delete char at cursor */
    } else if(kh == KEY_HI_DEL) {
      if(buff_cursor_offset < buff_size-1) {
        exmemcpy(buff, buff_cursor_offset, buff, buff_cursor_offset+1, buff_size-buff_cursor_offset-1);
        buff_size--;
        putchar_attr(strlen(argv[1]), 0, '*', TITLE_ATTRIBUTES);
      }

    /* Any other key but esc: insert char at cursor */
    } else if(kl != KEY_LO_ESC) {

      if(kl == KEY_LO_RETURN) {
        kl = '\n';
      }
      exmemcpy(buff, buff_cursor_offset+1, buff, buff_cursor_offset, buff_size-buff_cursor_offset);
      setexc(buff, buff_cursor_offset++, kl);
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
    set_show_cursor(HIDE_CURSOR);
    show_buffer_at_line(buff, current_line);
    line -= current_line;
    line += 1;
    set_cursor_position(col, line);
    set_show_cursor(SHOW_CURSOR);
  }

  /* Free buffer */
  exmfree(buff);

  /* Reset screen */
  clear_screen();
  set_cursor_position(0, 0);

  return 0;
}
