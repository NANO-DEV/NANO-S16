/*
 * User Library
 */

#ifndef _ULIB_H
#define _ULIB_H

/*
 * Some macros
 */
#define min(a,b) (a<b?a:b)
#define max(a,b) (a>b?a:b)

/*
 * System call
 * Avoid using it since there are already implemented
 * more general purpose functions in this library
 */
uint syscall(uint service, void* param);


/*
 * Get HIGH byte
 */
uchar getHI(uint c);
/*
 * Get LOW byte
 */
uchar getLO(uint c);


/*
 * This library expects:
 *
 * - Strings defined as uchar*
 * - Strings are finished with a 0
 * - New line is defined as "\n\r"
 */


/*
 * Send a character to the serial port
 */
void sputchar(uchar c);

/*
 * Send a formatted string to the serial port
 * Supports:
 * %d (int), %u (uint), %x (uint), %s (char*) and %U (uint32_t)
 */
void sputstr(uchar* format, ...);


/*
 * Attribute flags for function putchar_attr
 * AT_T_ is for text color
 * AT_B_ is for background color
 */
#define AT_T_BLACK    0x00
#define AT_T_BLUE     0x01
#define AT_T_GREEN    0x02
#define AT_T_CYAN     0x03
#define AT_T_RED      0x04
#define AT_T_MAGENTA  0x05
#define AT_T_BROWN    0x06
#define AT_T_LGRAY    0x07
#define AT_T_DGRAY    0x08
#define AT_T_LBLUE    0x09
#define AT_T_LGREEN   0x0A
#define AT_T_LCYAN    0x0B
#define AT_T_LRED     0x0C
#define AT_T_LMAGENTA 0x0D
#define AT_T_YELLOW   0x0E
#define AT_T_WHITE    0x0F

#define AT_B_BLACK    0x00
#define AT_B_BLUE     0x10
#define AT_B_GREEN    0x20
#define AT_B_CYAN     0x30
#define AT_B_RED      0x40
#define AT_B_MAGENTA  0x50
#define AT_B_BROWN    0x60
#define AT_B_LGRAY    0x70
#define AT_B_DGRAY    0x80
#define AT_B_LBLUE    0x90
#define AT_B_LGREEN   0xA0
#define AT_B_LCYAN    0xB0
#define AT_B_LRED     0xC0
#define AT_B_LMAGENTA 0xD0
#define AT_B_YELLOW   0xE0
#define AT_B_WHITE    0xF0

/*
 * Video mode screen size
 *
 * Screen positions are referred as x,y in this library.
 * x: column in screen. From 0 (leftmost) to SCREEN_WIDTH
 * y: line in screen. From 0 (topmost) to SCREEN_HEIGHT
 */
#define SCREEN_WIDTH  80
#define SCREEN_HEIGHT 25

/*
 * Clears the screen
 */
void clear_screen();

/*
 * Send a character to the screen at cursor position
 */
void putchar(uchar c);

/*
 * Send a character to the screen at a given position and
 * specific attributes. See attribute flags above.
 *
 * Cursor will be relocated to x, y in the screen
 */
void putchar_attr(uint x, uint y, uchar c, uchar attr);

/*
 * Display complex string on the screen at cursor position
 * Supports:
 * %d (int), %u (uint), %x (uint), %s (char*) and %U (uint32_t)
 */
void putstr(uchar* format, ...);


/*
 * Get cursor position
 */
void get_cursor_position(uint* x, uint* y);
/*
 * Set cursor position
 */
void set_cursor_position(uint x, uint y);


/*
 * Special key codes
 *
 * getkey() function returns an uint
 * To check a KEY_LO_ code, compare lower byte
 * To check a KEY_HI_ code, compare higher byte
 * Alphanumeric and usual symbol keys are mapped to the
 * lower byte, and their key code is their char code.
 * Example:
 *
 * uint k = getkey();
 * if(getHI(k) == KEY_HI_DEL) ...
 * if(getLO(k) == 'a') ...
 */
#define KEY_LO_BACKSPACE 0x08
#define KEY_LO_RETURN    0x0D
#define KEY_LO_ESC       0x1B
#define KEY_HI_DEL       0x53
#define KEY_HI_END       0x4F
#define KEY_HI_HOME      0x47
#define KEY_HI_INS       0x52
#define KEY_HI_PG_DN     0x51
#define KEY_HI_PG_UP     0x49
#define KEY_HI_PRT_SC    0x72
#define KEY_HI_TAB       0x0F

#define KEY_HI_UP        0x48
#define KEY_HI_LEFT      0x4B
#define KEY_HI_RIGHT     0x4D
#define KEY_HI_DOWN      0x50

#define KEY_HI_F1        0x3B
#define KEY_HI_F2        0x3C
#define KEY_HI_F3        0x3D
#define KEY_HI_F4        0x3E
#define KEY_HI_F5        0x3F
#define KEY_HI_F6        0x40
#define KEY_HI_F7        0x41
#define KEY_HI_F8        0x42
#define KEY_HI_F9        0x43
#define KEY_HI_F10       0x44
#define KEY_HI_F11       0x85
#define KEY_HI_F12       0x86

/*
 * Get key press in a char. Blocking function.
 * Special keys are ignored
 */
uchar getchar();

/*
 * Get key press. Blocking function
 */
uint getkey();

/*
 * Get a string from user. Returns when RETURN key is
 * pressed or str contains max_count elements.
 * Unused str characters will be set to 0
 * Returns number of elements in str
 */
uint getstr(uchar* str, uint max_count);



/*
 * Copy string src to dest
 */
uint strcpy(uchar* dest, uchar* src);

/*
 * Copy n elements from string src to dest
 */
uint strncpy(uchar* dest, uchar* src, uint n);

/*
 * Concatenate string src to dest
 */
uint strcat(uchar* dest, uchar* src);

/*
 * Get string length
 */
uint strlen(uchar* str);

/*
 * Compare strings
 */
int strcmp(uchar* str1, uchar* str2);

/*
 * Compare n elements of strings
 */
int strncmp(uchar* str1, uchar* str2, uint n);

/*
 * Get first token in string
 *
 * src is the input untokenized string. It will be modified.
 * delim is the delimiter character to separate tokens.
 * The function returns a pointer to the start of first token in src.
 * All consecutive delimiters found immediately after the first token
 * will be replaced with 0, so return value is a 0 finished string with
 * the first token.
 * After execution, (*next) will point to the remaining string after the
 * first token and its final 0 (or 0s).
 *
 * To tokenize a full string, this function should be called several times,
 * until *next = 0
 */
uchar* strtok(uchar* src, uchar** next, uchar delim);

/*
 * Find a character in a string
 * Returns:
 * - 0 if not found
 * - index+1 of first c in src otherwise
 */
uint strchr(uchar* src, uchar c);



/*
 * Copy size bytes from src to dest
 */
uint memcpy(uchar *dest, uchar *src, uint size);

/*
 * Set size bytes from src to value
 */
uint memset(uchar *dest, uchar value, uint size);



/*
 * Allocate size bytes of contiguous memory
 */
void* malloc(uint size);
/*
 * Free allocated memory
 */
void mfree(void* ptr);


/*
 * File system related
 */

 /* Unless another thing is specified, all paths must be provided as
  * absolute or relative to the system disk.
  * When specified as absolute, they must begin with a disk identifier.
  * Possible disk identifiers are:
  * fd0 - First floppy disk
  * fd1 - Second floppy disk
  * hd0 - First hard disk
  * hd1 - Second hard disk
  *
  * Path components are separated with slashes '/'
  * The root directory of a disk can be omitted or referred as "."
  * when it's the last path component.
  *
  * Examples of valid paths:
  * fd0
  * hd0/.
  * hd0/documents/file.txt
  */

 /* When disks must be referenced by uint disk, valid values are:
  * fd0 : 0x00
  * fd1 : 0x01
  * hd0 : 0x80
  * hd1 : 0x81
  */

/* Special return codes */
#define ERROR_NOT_FOUND 0xFFFF
#define ERROR_EXISTS    0xFFFE

/* FS_ENTRY flags */
#define FST_DIR  0x01   /* Directory */
#define FST_FILE 0x02   /* File */

struct  FS_ENTRY {
  uchar name[15];
  uchar flags;
  uint  size; /* bytes for files, items for directories */
};

/* FS_INFO.fs_type types */
#define FS_TYPE_UNKNOWN 0x000
#define FS_TYPE_NSFS    0x001

struct FS_INFO {
  uchar name[4];
  uint      id;        /* Disk id code */
  uint      fs_type;
  uint32_t  fs_size;   /* MB */
  uint32_t  disk_size; /* MB */
};

/*
 * Get filesystem info
 * Output: info
 * disk_index is referred to the index of a disk on the currently
 * available disks list.
 * returns number of available disks
 */
uint get_fsinfo(uint disk_index, struct FS_INFO* info);

/*
 * Get filesystem entry
 * Output: entry
 * parent and/or disk can be UNKNOWN_VALUE if they are unknown.
 * In this case they will be deducted from path
 * Paths must be:
 * - absolute or relative to system disk if parent and/or disk are unknown
 * - relative to parent if parent index entry or disk id are provided
 * Returns ERROR_NOT_FOUND if error, entry index otherwise
 */
 #define UNKNOWN_VALUE 0xFFFF
uint get_entry(struct FS_ENTRY* entry, uchar* path, uint parent, uint disk);

/*
 * Read file
 * Output: buff
 * Reads count bytes of path file starting at byte offset inside this file.
 * Returns number of readed bytes or ERROR_NOT_FOUND
 */
uint read_file(uchar* buff, uchar* path, uint offset, uint count);

/*
 * Write file flags
 */
 #define FWF_CREATE   0x0001 /* Create if does not exist */
 #define FWF_TRUNCATE 0x0002 /* Truncate to last written position */
 /*
  * Write file
  * Writes count bytes of path file starting at byte offset inside this file.
  * If target file is not big enough, its size is increased.
  * Depending on flags, path file can be created or truncated.
  * Returns number of written bytes or ERROR_NOT_FOUND
  */
uint write_file(uchar* buff, uchar* path, uint offset, uint count, uint flags);

/*
 * Move entry
 * In the case of directories, they are recursively moved
 * Returns:
 * - ERROR_NOT_FOUND if source does not exist
 * - ERROR_EXISTS if destination exists
 * - another value otherwise
 */
uint move(uchar* srcpath, uchar* dstpath);

/*
 * Copy entry
 * In the case of directories, they are recursively copied
 * Returns:
 * - ERROR_NOT_FOUND if source does not exist
 * - ERROR_EXISTS if destination exists
 * - another value otherwise
 */
uint copy(uchar* srcpath, uchar* dstpath);

/*
 * Delete entry
 * In the case of directories, they are recursively deleted
 * Returns:
 * - ERROR_NOT_FOUND if path does not exist
 * - index of deleted entry otherwise
 */
uint delete(uchar* path);

/*
 * Create a directory
 * Returns:
 * - ERROR_NOT_FOUND if parent path does not exist
 * - ERROR_EXISTS if destination already exists
 * - index of created entry otherwise
 */
uint create_directory(uchar* path);

/*
 * List directory entries
 * Output: entry
 * Gets entry with index n in path directory
 * Returns:
 * - ERROR_NOT_FOUND if path does not exist
 * - number of elements in ths directory otherwise
 */
uint list(struct FS_ENTRY* entry, uchar* path, uint n);

/*
 * Create filesystem in disk
 * Deletes all files, creates NSFS filesystem
 * and adds a copy of the kernel
 * Returns 0 on success
 */
uint format(uint disk);


/*
 * Get current system date and time
 */
struct TIME {
  uint  year;
  uint  month;
  uint  day;
  uint  hour;
  uint  minute;
  uint  second;
 };

void time(struct TIME* t);



#endif   /* _ULIB_H */
