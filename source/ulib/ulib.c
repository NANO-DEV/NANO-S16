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
 * Format and output a string char by char
 * calling an outchar_function for each output char
 */
#define D_STR_SIZE 32
typedef void *outchar_function(uchar);
void format_str_outchar(uchar *format, uint* args, outchar_function outchar)
{
  while(*format) {
    if(*format == '%') {
      uchar digit[D_STR_SIZE];
      uint32_t value32 = *(uint32_t*)(args);
      uint32_t value = *(uint*)(args++);
      uint32_t is_negative = (value & 0x8000);
      uint32_t base = 0;
      uint  n_digits = 0;
      digit[D_STR_SIZE-1] = 0;

      format++;

      if(*format == 'd') {
        base = 10;
        if(is_negative) {
          value = -value;
        }
      } else if(*format == 'u') {
        base = 10;
      } else if(*format == 'U') {
        value = value32;
        base = 10;
        args++;
      } else if(*format == 'x') {
        base = 16;
        n_digits = 4;
      } else if(*format == 's') {
        while(*(uchar*)value) {
          outchar(*(uchar*)(value++));
        }
        format++;
      }

      if(base != 0) {
        uint n = 0;

        do {
          uint32_t d = (value % base);
          n++;
          digit[D_STR_SIZE-1-n] = (d<=9 ? '0'+d : 'A'+d-10);
          value = value / base;
        } while((value && n < D_STR_SIZE-1) || (n < n_digits));

        if(*format == 'x') {
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
        }
        format++;
      }
    } else {
      outchar(*(format++));
    }
  }
}

/*
 * Send a character to the serial port
 */
void sputchar(uchar c)
{
  syscall(SYSCALL_IO_OUT_CHAR_SERIAL, (void*)c);
}

/*
 * Send complex string to the serial port
 */
void sputstr(uchar *format, ...)
{
  format_str_outchar(format, &format + 1, sputchar);
}

/*
 * Get screen size
 */
void get_screen_size(uint* width, uint* height)
{
  struct TSYSCALL_POSITION ps;
  ps.x = 0;
  ps.y = 0;
  ps.px = width;
  ps.py = height;
  syscall(SYSCALL_IO_GET_SCREEN_SIZE, &ps);
}

/*
 * Clears the screen
 */
void clear_screen()
{
  syscall(SYSCALL_IO_CLEAR_SCREEN, 0);
}

/*
 * Send a character to the screen
 */
void putchar(uchar c)
{
  syscall(SYSCALL_IO_OUT_CHAR, (void*)c);
}

/*
 * Display formatted string on the screen
 */
void putstr(uchar *format, ...)
{
  format_str_outchar(format, &format + 1, putchar);
}

/*
 * Send a character to the screen with attr
 */
void putchar_attr(uint x, uint y, uchar c, uchar attr)
{
  struct TSYSCALL_CHARATTR ca;
  ca.x = x;
  ca.y = y;
  ca.c = c;
  ca.attr = attr;
  syscall(SYSCALL_IO_OUT_CHAR_ATTR, &ca);
}

/*
 * Get cursor position
 */
void get_cursor_position(uint* x, uint* y)
{
  struct TSYSCALL_POSITION ps;
  ps.x = 0;
  ps.y = 0;
  ps.px = x;
  ps.py = y;
  syscall(SYSCALL_IO_GET_CURSOR_POS, &ps);
}

/*
 * Set cursor position
 */
void set_cursor_position(uint x, uint y)
{
  struct TSYSCALL_POSITION ps;
  ps.x = x;
  ps.y = y;
  ps.px = 0;
  ps.py = 0;
  syscall(SYSCALL_IO_SET_CURSOR_POS, &ps);
}

/*
 * Get key press in a char
 */
uchar getchar()
{
  uint c = syscall(SYSCALL_IO_IN_KEY, 0);
  return (uchar)(c & 0x00FF);
}

/*
 * Get key press
 */
uint getkey()
{
  return syscall(SYSCALL_IO_IN_KEY, 0);
}

/*
 * Get a string from user
 */
uint getstr(uchar* str, uint max_count)
{
  uint i = 0;

  while(1) {
    uint c = getchar();
    if(c == KEY_LO_RETURN) {
      putchar('\n');
      putchar('\r');
      break;
    }
    if(c == KEY_LO_BACKSPACE) {
      if(i > 0) {
        i--;
        str[i] = 0;
        putchar(KEY_LO_BACKSPACE);
        putchar(0);
        putchar(KEY_LO_BACKSPACE);
      }
    } else if(c>=32 && c<=126 && i+1<max_count) {
      str[i] = c;
      putchar(str[i]);
      i++;
    }
  }
  str[i] = 0;

  return i;
}

/*
 * Copy string src to dest
 */
uint strcpy(uchar *dest, uchar *src)
{
  uint i = 0;
  while(src[i] != 0) {
    dest[i] = src[i];
    i++;
  }
  dest[i] = 0;
  return i;
}

/*
 * Copy string src to dest without exceeding
 * dest_size elements in dest
 */
uint strcpy_s(uchar *dest, uchar *src, uint dest_size)
{
  uint i = 0;
  while(src[i]!=0 && i+1<dest_size) {
    dest[i] = src[i];
    i++;
  }
  dest[i] = 0;
  return i;
}

/*
 * Concatenate string src to dest
 */
uint strcat(uchar *dest, uchar *src)
{
  uint j = 0;
  uint i = strlen(dest);
  while(src[j] != 0) {
    dest[i] = src[j];
    i++;
    j++;
  }
  dest[i] = 0;
  return i;
}

/*
 * Concatenate string src to dest, without exceeding
 * dest_size elements in dest
 */
uint strcat_s(uchar* dest, uchar* src, uint dest_size)
{
  uint j = 0;
  uint i = strlen(dest);
  while(src[j]!=0 && i+1<dest_size) {
    dest[i] = src[j];
    i++;
    j++;
  }
  dest[i] = 0;
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
 * Copy size bytes from src to dest
 */
uint memcpy(uchar* dest, uchar* src, uint size)
{
  uint i = 0;
  for(i=0; i<size; i++) {
    dest[i] = src[i];
  }
  return i;
}

/*
 * Set size bytes from dest to value
 */
uint memset(uchar* dest, uchar value, uint size)
{
  uint i = 0;
  for(i=0; i<size; i++) {
    dest[i] = value;
  }
  return i;
}

/*
 * Allocate memory block
 */
void* malloc(uint size)
{
  return (void*)syscall(SYSCALL_MEM_ALLOCATE, &size);
}

/*
 * Free memory block
 */
void mfree(void* ptr)
{
  syscall(SYSCALL_MEM_FREE, ptr);
}

/*
 * Get filesystem info
 */
uint get_fsinfo(uint disk_index, struct FS_INFO* info)
{
  struct TSYSCALL_FSINFO fi;
  fi.disk_index = disk_index;
  fi.info = info;
  return syscall(SYSCALL_FS_GET_INFO, &fi);
}

/*
 * Get filesystem entry
 */
uint get_entry(struct FS_ENTRY* entry, uchar* path, uint parent, uint disk)
{
  struct TSYSCALL_FSENTRY fi;
  fi.entry = entry;
  fi.path = path;
  fi.parent = parent;
  fi.disk = disk;
  return syscall(SYSCALL_FS_GET_ENTRY, &fi);
}

/*
 * Read file
 */
uint read_file(uchar* buff, uchar* path, uint offset, uint count)
{
  struct TSYSCALL_FSRWFILE fi;
  fi.buff = buff;
  fi.path = path;
  fi.offset = offset;
  fi.count = count;
  fi.flags = 0;
  return syscall(SYSCALL_FS_READ_FILE, &fi);
}

/*
 * Write file
 */
uint write_file(uchar* buff, uchar* path, uint offset, uint count, uint flags)
{
  struct TSYSCALL_FSRWFILE fi;
  fi.buff = buff;
  fi.path = path;
  fi.offset = offset;
  fi.count = count;
  fi.flags = flags;
  return syscall(SYSCALL_FS_WRITE_FILE, &fi);
}

/*
 * Move entry
 */
uint move(uchar* srcpath, uchar* dstpath)
{
  struct TSYSCALL_FSSRCDST fi;
  fi.src = srcpath;
  fi.dst = dstpath;
  return syscall(SYSCALL_FS_MOVE, &fi);
}

/*
 * Copy entry
 */
uint copy(uchar* srcpath, uchar* dstpath)
{
  struct TSYSCALL_FSSRCDST fi;
  fi.src = srcpath;
  fi.dst = dstpath;
  return syscall(SYSCALL_FS_COPY, &fi);
}

/*
 * Delete entry
 */
uint delete(uchar* path)
{
  return syscall(SYSCALL_FS_DELETE, path);
}

/*
 * Create a directory
 */
uint create_directory(uchar* path)
{
  return syscall(SYSCALL_FS_CREATE_DIRECTORY, path);
}

/*
 * List dir entries
 */
uint list(struct FS_ENTRY* entry, uchar* path, uint n)
{
  struct TSYSCALL_FSLIST fi;
  fi.entry = entry;
  fi.path = path;
  fi.n = n;
  return syscall(SYSCALL_FS_LIST, &fi);
}

/*
 * Create filesystem in disk
 */
uint format(uint disk)
{
  return syscall(SYSCALL_FS_FORMAT, disk);
}

/*
 * Get system date and time
 */
void time(struct TIME* t)
{
  syscall(SYSCALL_CLK_GET_TIME, t);
}
