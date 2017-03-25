/*
 * User Lib
 */

#include "types.h"
#include "syscall.h"
#include "ulib.h"

/*
 * Most functions defined here are just
 * parameter packing and performing a system call.
 * More detailed descriptions can be found at ulib.h
 */

/*
 * Get higher byte from uint
 */
uchar getHI(uint c)
{
  return (c>>8) & 0xFF;
}

/*
 * Get lower byte from uint
 */
uchar getLO(uint c)
{
  return (c & 0xFF);
}

/*
 * Generate "random" number
 * PRNG based on the middle-square method
 */
uint rand()
{
  static ul_t seed = 0;

  if(seed == 0) {
    syscall(SYSCALL_CLK_GET_MILISEC, lp(&seed));
    seed += 0x11111111L;
  }

  seed = seed*seed;
  seed &= 0x00FFFF00L;
  seed = seed >> 8;

  return (uint)seed;
}

/*
 * Format and output a string char by char
 * calling an outchar_function for each output char
 */
#define D_STR_SIZE 32
typedef void *outchar_function(uchar);
void format_str_outchar(uchar* format, uint* args, outchar_function outchar)
{
  uint char_count = 0;

  while(*format) {
    if(*format == '%') {
      uchar digit[D_STR_SIZE];
      ul_t value32 = *(ul_t*)(args);
      ul_t value = *(uint*)(args++);
      ul_t is_negative = (value & 0x8000);
      ul_t base = 0;
      uint n_digits = 0;
      digit[D_STR_SIZE-1] = 0;

      format++;

      while(*format>='0' && *format<='9') {
        n_digits=n_digits*10 + *format-'0';
        format++;
      }

      if(*format == 'd') {
        base = 10;
        if(is_negative) {
          value = -(int)value;
        }
      } else if(*format == 'u') {
        base = 10;
      } else if(*format == 'U') {
        value = value32;
        base = 10;
        args++;
      } else if(*format == 'x') {
        base = 16;
        n_digits = n_digits?n_digits:4;
      } else if(*format == 'X') {
        value = value32;
        base = 16;
        n_digits = n_digits?n_digits:8;
        args++;
      } else if(*format == 's') {
        while(*(uchar*)value) {
          outchar(*(uchar*)(value++));
          char_count++;
        }
        format++;
      } else if(*format == 'c') {
        if(n_digits) {
          while(char_count<n_digits) {
            outchar((uchar)value);
            char_count++;
          }
        } else {
          outchar((uchar)value);
          char_count++;
        }
        format++;
      }

      if(base != 0) {
        uint n = 0;

        do {
          ul_t d = (value % base);
          n++;
          digit[D_STR_SIZE-1-n] = (d<=9 ? '0'+d : 'A'+d-10);
          value = value / base;
        } while((value && n < D_STR_SIZE-1) || (n < n_digits));

        if(*format == 'x' || *format == 'X') {
          n++;
          digit[D_STR_SIZE-1-n] = 'x';
          n++;
          digit[D_STR_SIZE-1-n] = '0';
        } else if(*format == 'd' && is_negative) {
          n++;
          digit[D_STR_SIZE-1-n] = '-';
        }

        value = &(digit[D_STR_SIZE-1-n]);
        while(*(uchar*)value) {
          outchar(*(uchar*)(value++));
          char_count++;
        }
        format++;
      }
    } else {
      outchar(*(format++));
      char_count++;
    }
  }
}

/*
 * Format a string
 */
static uchar* ststr;
static void stcatchar(uchar c) { ststr[strlen(ststr)] = c; }
void formatstr(uchar* str, uint size, uchar* format, ...)
{
  ststr = str;
  memset(ststr, 0, size);
  format_str_outchar(format, &format+1, stcatchar);
}

/*
 * Get mouse state
 */
void get_mouse_state(uint mode, uint* x, uint* y, uint* b)
{
  struct TSYSCALL_POSATTR pa;
  pa.attr = mode;
  syscall(SYSCALL_IO_GET_MOUSE_STATE, lp(&pa));
  *x = pa.x;
  *y = pa.y;
  *b = pa.c;
}

/*
 * Send a character to the serial port
 */
void sputchar(uchar c)
{
  syscall(SYSCALL_IO_OUT_CHAR_SERIAL, lp(&c));
}

/*
 * Send complex string to the serial port
 */
void sputstr(uchar* format, ...)
{
  format_str_outchar(format, &format+1, sputchar);
}

/*
 * Get char from serial port
 */
uchar sgetchar()
{
  return (uchar)syscall(SYSCALL_IO_IN_CHAR_SERIAL, 0L);
}

/*
 * Send a character to the debug output
 */
void debugchar(uchar c)
{
  syscall(SYSCALL_IO_OUT_CHAR_DEBUG, lp(&c));
}

/*
 * Send a complex string on the debug output
 */
void debugstr(uchar* format, ...)
{
  format_str_outchar(format, &format+1, debugchar);
}

/*
 * Get video mode
 */
uint get_video_mode()
{
  return syscall(SYSCALL_IO_GET_VIDEO_MODE, 0L);
}

/*
 * Set video mode
 */
void set_video_mode(uint mode)
{
  syscall(SYSCALL_IO_SET_VIDEO_MODE, lp(&mode));
}

/*
 * Get screen size
 */
void get_screen_size(uint mode, uint* width, uint* height)
{
  struct TSYSCALL_POSITION ps;
  ps.x = mode;
  ps.y = 0;
  ps.px = lp(width);
  ps.py = lp(height);
  syscall(SYSCALL_IO_GET_SCREEN_SIZE, lp(&ps));
}

/*
 * Clears the screen
 */
void clear_screen()
{
  syscall(SYSCALL_IO_CLEAR_SCREEN, 0L);
}

/*
 * Draw a pixel in graphics mode
 */
void set_pixel(uint x, uint y, uint color)
{
  struct TSYSCALL_POSATTR ca;
  ca.x = x;
  ca.y = y;
  ca.c = color;
  ca.attr = 0;
  syscall(SYSCALL_IO_SET_PIXEL, lp(&ca));
}

/*
 * Draw a char in graphics mode
 */
void draw_char(uint x, uint y, uint c, uint color)
{
  struct TSYSCALL_POSATTR ca;
  ca.x = x;
  ca.y = y;
  ca.c = c;
  ca.attr = color;
  syscall(SYSCALL_IO_DRAW_CHAR, lp(&ca));
}

/*
 * Draw a map in graphics mode
 */
void draw_map(uint x, uint y, uchar* buff, uint width, uint height)
{
  struct TSYSCALL_POSATTR ca;
  uint i, j;
  for(j=0; j<height; j++) {
    for(i=0; i<width; i++) {
      ca.x = x+i;
      ca.y = y+j;
      ca.c = buff[j*width + i];
      ca.attr = 0;
      syscall(SYSCALL_IO_SET_PIXEL, lp(&ca));
    }
  }
}

/*
 * Send a character to the screen
 */
void putchar(uchar c)
{
  syscall(SYSCALL_IO_OUT_CHAR, lp(&c));
}

/*
 * Display formatted string on the screen
 */
void putstr(uchar* format, ...)
{
  format_str_outchar(format, &format+1, putchar);
}

/*
 * Send a character to the screen with attr
 */
void putchar_attr(uint col, uint row, uchar c, uchar attr)
{
  struct TSYSCALL_POSATTR ca;
  ca.x = col;
  ca.y = row;
  ca.c = c;
  ca.attr = attr;
  syscall(SYSCALL_IO_OUT_CHAR_ATTR, lp(&ca));
}

/*
 * Get cursor position
 */
void get_cursor_position(uint* col, uint* row)
{
  struct TSYSCALL_POSITION ps;
  ps.x = 0;
  ps.y = 0;
  ps.px = lp(col);
  ps.py = lp(row);
  syscall(SYSCALL_IO_GET_CURSOR_POS, lp(&ps));
}

/*
 * Set cursor position
 */
void set_cursor_position(uint col, uint row)
{
  struct TSYSCALL_POSITION ps;
  ps.x = col;
  ps.y = row;
  ps.px = 0;
  ps.py = 0;
  syscall(SYSCALL_IO_SET_CURSOR_POS, lp(&ps));
}

/*
 * Set cursor visibility
 */
void set_show_cursor(uint mode)
{
  syscall(SYSCALL_IO_SET_SHOW_CURSOR, lp(&mode));
}

/*
 * Get key press in a char
 */
uchar getchar()
{
  uint m = KM_WAIT_KEY;
  uint c = syscall(SYSCALL_IO_IN_KEY, lp(&m));
  return (uchar)(c & 0x00FF);
}

/*
 * Get key press
 */
uint getkey(uint mode)
{
  return syscall(SYSCALL_IO_IN_KEY, lp(&mode));
}

/*
 * Get a string from user
 */
uint getstr(uchar* str, uint max_count)
{
  uint col, row;
  uint i = 0;
  uint k;
  uint p;

  /* Reset string */
  memset(str, 0, max_count);

  /* Clear keyboard buffer */
  getkey(KM_CLEAR_BUFFER);

  /* Get cursor position and show it */
  get_cursor_position(&col, &row);
  set_show_cursor(SHOW_CURSOR);

  do {
    /* Get a key press (wait for it)*/
    k = getkey(KM_NO_WAIT);

    /* Backspace */
    if(k == KEY_BACKSPACE) {
      if(i > 0) {
        memcpy(&str[i-1], &str[i], max_count-i);
        i--;
      }

    /* Delete */
  } else if(k == KEY_DEL) {
      if(i < max_count-1) {
        memcpy(&str[i], &str[i+1], max_count-i-1);
      }

    /* Return */
  } else if(k == KEY_RETURN) {
      break;

    /* Cursor movement keys */
  } else if(k == KEY_LEFT && i>0) {
      i--;
    } else if(k == KEY_RIGHT && i<strlen(str)) {
      i++;
    } else if(k == KEY_HOME) {
      i = 0;
    } else if(k == KEY_END) {
      i = strlen(str);

    /* Not a useful key and not an allowed char */
  } else if(k!=KEY_TAB && (k<0x20 || k>0x7E)) {
      continue;

    /* Append char to string */
    } else if(strlen(str) < max_count-1) {
      memcpy(&str[i+1], &str[i], max_count-i-2);
      str[i++] = getLO(k);
    }

    /* Now hide cursor and redraw string at initial position */
    set_show_cursor(HIDE_CURSOR);
    set_cursor_position(col, row);
    for(p=0; p<max_count; p++) {
      putchar(str[p]);
    }

    /* Reset cursor */
    set_cursor_position(col+i, row);
    set_show_cursor(SHOW_CURSOR);
  } while(k != KEY_RETURN);

  /* Done. Print new line */
  str[max_count-1] = 0;
  putchar('\n');
  putchar('\r');

  return strlen(str);
}

/*
 * Copy string src to dst
 */
uint strcpy(uchar* dst, uchar* src)
{
  uint i = 0;
  while(src[i] != 0) {
    dst[i] = src[i];
    i++;
  }
  dst[i] = 0;
  return i;
}

/*
 * Copy string src to dst without exceeding
 * dst_size elements in dst
 */
uint strcpy_s(uchar* dst, uchar* src, uint dst_size)
{
  uint i = 0;
  while(src[i]!=0 && i+1<dst_size) {
    dst[i] = src[i];
    i++;
  }
  dst[i] = 0;
  return i;
}

/*
 * Concatenate string src to dst
 */
uint strcat(uchar* dst, uchar* src)
{
  uint j = 0;
  uint i = strlen(dst);
  while(src[j] != 0) {
    dst[i] = src[j];
    i++;
    j++;
  }
  dst[i] = 0;
  return i;
}

/*
 * Concatenate string src to dst, without exceeding
 * dst_size elements in dst
 */
uint strcat_s(uchar* dst, uchar* src, uint dst_size)
{
  uint j = 0;
  uint i = strlen(dst);
  while(src[j]!=0 && i+1<dst_size) {
    dst[i] = src[j];
    i++;
    j++;
  }
  dst[i] = 0;
  return i;
}

/*
 * Get string length
 */
uint strlen(uchar* str)
{
  uint i = 0;
  while(str[i] != 0) {
    i++;
  }
  return i;
}

/*
 * Compare strings
 */
int strcmp(uchar* str1, uchar* str2)
{
  uint i = 0;
  while(str1[i]==str2[i] && str1[i]!=0) {
    i++;
  }
  return str1[i] - str2[i];
}

/*
 * Compare n elements of strings
 */
int strncmp(uchar* str1, uchar* str2, uint n)
{
  uint i = 0;
  while(str1[i]==str2[i] && str1[i]!=0 && i+1<n) {
    i++;
  }
  return str1[i] - str2[i];
}

/*
 * Tokenize string
 */
uchar* strtok(uchar* src, uchar** next, uchar delim)
{
  uchar* s;

  while(*src == delim) {
    *src = 0;
    src++;
  }

  s = src;

  while(*s) {
    if(*s == delim) {
      *s = 0;
      *next = s+1;
      return src;
    }
    s++;
  }

  *next = s;
  return src;
}

/*
* Find char in string
*/
uint strchr(uchar* src, uchar c)
{
  uint n = 0;
  while(src[n]) {
    if(src[n] == c) {
      return n+1;
    }
    n++;
  }
  return 0;
}

/*
 * Parse string uint
 */
uint stou(uchar* src)
{
  uint base = 10;
  uint value = 0;

  /* Is hex? */
  if(src[0]=='0' && src[1]=='x') {
    src += 2;
    base = 16;
  }

  while(*src) {
    uint digit = *src<='9'? *src-'0' :
      *src>='a'? 0xA+*src-'a' : 0xA+*src-'A';

    value = value*base + digit;
    src++;
  }
  return value;
}

/*
 * Is string uint?
 */
uint sisu(uchar* src)
{
  uint base = 10;

  if(!src[0]) {
    return 0;
  }

  /* Is hex? */
  if(src[0]=='0' && src[1]=='x' && src[2]) {
    src += 2;
    base = 16;
  }

  while(*src) {
    uint digit = *src<='9'? *src-'0' :
      *src>='a'? 0xA+*src-'a' : 0xA+*src-'A';

    if(digit>=base) {
      return 0;
    }
    src++;
  }
  return 1;
}

/*
 * Copy size bytes from src to dst
 */
uint memcpy(uchar* dst, uchar* src, uint size)
{
  uint i = 0;
  uint rdir = (src>dst)?0:1;

  for(i=0; i<size; i++) {
    uint c = rdir?size-(i+1):i;
    dst[c] = src[c];
  }
  return i;
}

/*
 * Set size bytes from dst to value
 */
uint memset(uchar* dst, uchar value, uint size)
{
  uint i = 0;
  for(i=0; i<size; i++) {
    dst[i] = value;
  }
  return i;
}

/*
 * Compare size bytes from mem1 and mem2
 */
uint memcmp(uchar* mem1, uchar* mem2, uint size)
{
  uint i = 0;
  for(i=0; i<size; i++) {
    if(mem1[i] != mem2[i]) {
      return mem1[i]-mem2[i];
    }
  }
  return 0;
}

/*
 * Allocate memory block
 */
void* malloc(uint size)
{
  return (void*)syscall(SYSCALL_MEM_ALLOCATE, lp(&size));
}

/*
 * Free memory block
 */
void mfree(void* ptr)
{
  syscall(SYSCALL_MEM_FREE, (lp_t)ptr);
}

/*
 * Copy size bytes from src to dest (far memory)
 */
ul_t lmemcpy(lp_t dst, lp_t src, ul_t size)
{
  ul_t i = 0;
  uint rdir;
  struct TSYSCALL_LMEM ldst;
  struct TSYSCALL_LMEM lsrc;
  lsrc.n = 0;

  if(src > dst) {
    rdir = 0;
  } else {
    rdir = 1;
  }

  for(i=0; i<size; i++) {
    lp_t c = rdir ? size-1L-i : i;
    lsrc.dst = src + c;
    ldst.dst = dst + c;
    ldst.n = syscall(SYSCALL_LMEM_GET, lp(&lsrc));
    syscall(SYSCALL_LMEM_SET, lp(&ldst));
  }

  return i;
}

/*
 * Set size bytes from src to value (far memory)
 */
ul_t lmemset(lp_t dest, uchar value, ul_t size)
{
  ul_t i = 0;
  struct TSYSCALL_LMEM lm;
  lm.n = value;

  for(i=0; i<size; i++) {
    lm.dst = dest + i;
    syscall(SYSCALL_LMEM_SET, lp(&lm));
  }
  return i;
}

/*
 * Allocate size bytes of contiguous far memory
 */
lp_t lmalloc(ul_t size)
{
  struct TSYSCALL_LMEM lm;
  lm.dst = 0;
  lm.n = size;
  syscall(SYSCALL_LMEM_ALLOCATE, lp(&lm));
  return lm.dst;
}

/*
 * Free allocated far memory
 */
void lmfree(lp_t ptr)
{
  struct TSYSCALL_LMEM lm;
  lm.dst = ptr;
  lm.n = 0;
  syscall(SYSCALL_LMEM_FREE, lp(&lm));
}

/*
 * Get filesystem info
 */
uint get_fsinfo(uint disk_index, struct FS_INFO* info)
{
  struct TSYSCALL_FSINFO fi;
  fi.disk_index = disk_index;
  fi.info = lp(info);
  return syscall(SYSCALL_FS_GET_INFO, lp(&fi));
}

/*
 * Get filesystem entry
 */
uint get_entry(struct FS_ENTRY* entry, uchar* path, uint parent, uint disk)
{
  struct TSYSCALL_FSENTRY fi;
  fi.entry = lp(entry);
  fi.path = lp(path);
  fi.parent = parent;
  fi.disk = disk;
  return syscall(SYSCALL_FS_GET_ENTRY, lp(&fi));
}

/*
 * Read file
 */
uint read_file(uchar* buff, uchar* path, uint offset, uint count)
{
  struct TSYSCALL_FSRWFILE fi;
  fi.buff = lp(buff);
  fi.path = lp(path);
  fi.offset = offset;
  fi.count = count;
  fi.flags = 0;
  return syscall(SYSCALL_FS_READ_FILE, lp(&fi));
}

/*
 * Write file
 */
uint write_file(uchar* buff, uchar* path, uint offset, uint count, uint flags)
{
  struct TSYSCALL_FSRWFILE fi;
  fi.buff = lp(buff);
  fi.path = lp(path);
  fi.offset = offset;
  fi.count = count;
  fi.flags = flags;
  return syscall(SYSCALL_FS_WRITE_FILE, lp(&fi));
}

/*
 * Move entry
 */
uint move(uchar* srcpath, uchar* dstpath)
{
  struct TSYSCALL_FSSRCDST fi;
  fi.src = lp(srcpath);
  fi.dst = lp(dstpath);
  return syscall(SYSCALL_FS_MOVE, lp(&fi));
}

/*
 * Copy entry
 */
uint copy(uchar* srcpath, uchar* dstpath)
{
  struct TSYSCALL_FSSRCDST fi;
  fi.src = lp(srcpath);
  fi.dst = lp(dstpath);
  return syscall(SYSCALL_FS_COPY, lp(&fi));
}

/*
 * Delete entry
 */
uint delete(uchar* path)
{
  return syscall(SYSCALL_FS_DELETE, lp(path));
}

/*
 * Create a directory
 */
uint create_directory(uchar* path)
{
  return syscall(SYSCALL_FS_CREATE_DIRECTORY, lp(path));
}

/*
 * List dir entries
 */
uint list(struct FS_ENTRY* entry, uchar* path, uint n)
{
  struct TSYSCALL_FSLIST fi;
  fi.entry = lp(entry);
  fi.path = lp(path);
  fi.n = n;
  return syscall(SYSCALL_FS_LIST, lp(&fi));
}

/*
 * Create filesystem in disk
 */
uint format(uint disk)
{
  return syscall(SYSCALL_FS_FORMAT, lp(&disk));
}

/*
 * Get system date and time
 */
void time(struct TIME* t)
{
  syscall(SYSCALL_CLK_GET_TIME, lp(t));
}

/*
 * Get system timer, miliseconds
 */
ul_t get_timer()
{
  ul_t timer_ms;
  syscall(SYSCALL_CLK_GET_MILISEC, lp(&timer_ms));
  return timer_ms;
}

/*
 * Wait an amount of miliseconds
 */
void wait(uint miliseconds)
{
  ul_t initial_timer, timer;
  syscall(SYSCALL_CLK_GET_MILISEC, lp(&initial_timer));
  timer = initial_timer;
  while(timer < initial_timer + (ul_t)miliseconds) {
    syscall(SYSCALL_CLK_GET_MILISEC, lp(&timer));
  }
}

/*
 * Convert string to IP
 */
void str_to_ip(uint8_t* ip, uchar* str)
{
  uint i = 0;
  uchar tok_str[24];
  uchar* tok = tok_str;
  uchar* nexttok = tok;
  strcpy_s(tok_str, str, sizeof(tok_str));

  /* Tokenize */
  while(*tok && *nexttok && i<4) {
    tok = strtok(tok, &nexttok, '.');
    if(*tok) {
      ip[i++] = stou(tok);
   }
   tok = nexttok;
  }
}

/*
 * Convert IP to string
 */
uchar* ip_to_str(uchar* str, uint8_t* ip)
{
  formatstr(str, 16, "%u.%u.%u.%u", ip[0], ip[1], ip[2], ip[3]);
  return str;
}

/*
 * Get and remove from buffer received data.
 * src_ip and buff are filled by the function
 */
uint recv(uint8_t* src_ip, uchar* buff, uint buff_size)
{
  struct TSYSCALL_NETOP no;
  no.addr = lp(src_ip);
  no.buff = lp(buff);
  no.size = buff_size;
  return syscall(SYSCALL_NET_RECV, lp(&no));
}

/*
 * Send buffer to dst_ip
 */
uint send(uint8_t* dst_ip, uchar* buff, uint len)
{
  struct TSYSCALL_NETOP no;
  no.addr = lp(dst_ip);
  no.buff = lp(buff);
  no.size = len;
  return syscall(SYSCALL_NET_SEND, lp(&no));
}
