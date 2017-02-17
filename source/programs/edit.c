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

/*
 * Given a string (uchar* line input parameter)
 * returns a pointer to next line or to end of string
 * if there are not more lines
 *
 * Initial line of input string can be:
 * - shown at given screen_y if mode==SHOW_CURRENT
 * - just skipped if mode==SKIP_CURRENT
 */
static uchar* next_line(uint mode, uint screen_y, uchar* line)
{
  uint x = 0;

  /* Process string until next line or end of string
   * and show text if needed
   */
  while(line && *line && *line!='\n' && x<SCREEN_WIDTH) {
    if(mode == SHOW_CURRENT) {
      putchar_attr(x++, screen_y, *line, EDITOR_ATTRIBUTES);
    }
    line++;
  }

  /* Clear the rest of this line with spaces */
  while(x<SCREEN_WIDTH && mode == SHOW_CURRENT) {
    putchar_attr(x++, screen_y, ' ', EDITOR_ATTRIBUTES);
  }

  /* Advance until next line begins */
  if(line && *line == '\n') {
    line++;
  }

  return line;
}

/*
 * Shows buffer in editor area starting at a given
 * number of line of buffer
 */
void show_buffer_at_line(uchar* buff, uint n)
{
  uint l = 0;

  /* Hide cursor while drawing */
  set_cursor_position(0xFFFF, 0xFFFF);

  /* Skip buffer until required line number */
  while(l<n && *buff) {
    buff = next_line(SKIP_CURRENT, 0, buff);
    l++;
  }

  /* Show required buffer lines */
  for(l=1; l<SCREEN_HEIGHT; l++) {
    if(*buff) {
      buff = next_line(SHOW_CURRENT, l, buff);
    } else {
      next_line(SHOW_CURRENT, l, 0);
    }
  }
}

/*
 * Convert linear buffer offset (uint offset)
 * to line and col (output params)
 */
void buffer_offset_to_linecol(uchar* buff, uint offset, uint* col, uint* line)
{
  uint c;
  *col = 0;
  *line = 0;

  for(c=0; c<offset && buff[c]; c++) {

    if(buff[c] == '\n') {
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
uint linecol_to_buffer_offset(uchar* buff, uint col, uint line)
{
  uint offset = 0;
  while(buff[offset] && line>0) {
    if(buff[offset] == '\n') {
      line--;
    }
    offset++;
  }

  while(buff[offset] && col>0) {
    if(buff[offset] == '\n') {
      break;
    }
    col--;
    offset++;
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

  /* buff is dinamically allocated and
   * always contains the full file.
   * buff_size is the size in bytes of allocated buff
   * buff_cursor_offset is the linear offset of current
   * cursor position inside buff
   */
  uchar* buff = 0;
  uint   buff_size = 0;
  uint   buff_cursor_offset = 0;

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

  /* Find file */
  n = get_entry(&entry, argv[1], UNKNOWN_VALUE, UNKNOWN_VALUE);

  /* Load file or show error */
  if(n!=ERROR_NOT_FOUND && (entry.flags & FST_FILE)) {

    /* +1 because maybe the file is empty */
    /* or we need to append a 0 at the end */
    buff = malloc(entry.size + 1);
    if(buff != 0) {
      buff[entry.size] = 0;
      buff_size = entry.size;
      result = read_file(buff, argv[1], 0, entry.size);
      if(result != entry.size) {
        mfree(buff);
        putstr("Can't read file %s (error=%x)\n\r", argv[1], result);
        return 1;
      }
      /* Buffer must finish with a 0 and */
      /* must fit at least this 0, so buff_size can't be 0 */
      if(buff_size == 0 || buff[buff_size-1] != 0) {
        /* buff_size can be increased without resizing the buffer */
        /* because entry.size + 1 bytes were actually allocated */
        /* but buff_size was initialized to only entry.size */
        buff_size++;
      }
    } else {
      putstr("Not enough memory\n\r");
      return 1;
    }
  }

  /* Create 1 byte buffer if this is a new file */
  /* This byte is for the final 0 */
  if(buff == 0) {
    buff = malloc(1);
    buff_size = 1;
    memset(buff, 0, buff_size);
  }

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
  show_buffer_at_line(buff, current_line);
  set_cursor_position(0, 1);

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
      write_file(buff, argv[1], 0, buff_size, FWF_CREATE | FWF_TRUNCATE);
      putchar_attr(strlen(argv[1]), 0, ' ', TITLE_ATTRIBUTES);


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
        uchar* new_buff = malloc(buff_size - 1);
        memcpy(new_buff, buff, buff_cursor_offset - 1);
        memcpy(&new_buff[buff_cursor_offset-1], &buff[buff_cursor_offset], buff_size - buff_cursor_offset);
        mfree(buff);
        buff_size--;
        buff = new_buff;
        putchar_attr(strlen(argv[1]), 0, '*', TITLE_ATTRIBUTES);
        buff_cursor_offset--;
      }

    /* Del key: delete char at cursor */
    } else if(kh == KEY_HI_DEL) {
      if(buff_cursor_offset < buff_size - 1) {
        uchar* new_buff = malloc(buff_size - 1);
        memcpy(new_buff, buff, buff_cursor_offset);
        memcpy(&new_buff[buff_cursor_offset], &buff[buff_cursor_offset+1], buff_size - buff_cursor_offset - 1);
        mfree(buff);
        buff_size--;
        buff = new_buff;
        putchar_attr(strlen(argv[1]), 0, '*', TITLE_ATTRIBUTES);
      }

    /* Any other key but esc: insert char at cursor */
    } else if(kl != KEY_LO_ESC) {
      uchar* new_buff = malloc(buff_size + 1);
      memcpy(new_buff, buff, buff_cursor_offset);

      if(kl == KEY_LO_RETURN) {
        kl = '\n';
      }

      new_buff[buff_cursor_offset++] = kl;
      memcpy(&new_buff[buff_cursor_offset], &buff[buff_cursor_offset-1], buff_size + 1 - buff_cursor_offset);
      mfree(buff);
      buff_size++;
      buff = new_buff;
      putchar_attr(strlen(argv[1]), 0, '*', TITLE_ATTRIBUTES);
    }

    /* Update cursor position and display */
    buffer_offset_to_linecol(buff, buff_cursor_offset, &col, &line);
    if(line < current_line) {
      current_line = line;
    } else if(line > current_line + SCREEN_HEIGHT - 2) {
      current_line = line - SCREEN_HEIGHT + 2;
    }
    show_buffer_at_line(buff, current_line);
    line -= current_line;
    line += 1;
    set_cursor_position(col, line);
  }

  /* Free buffer */
  mfree(buff);

  /* Reset screen */
  clear_screen();
  set_cursor_position(0, 0);

  return 0;
}
