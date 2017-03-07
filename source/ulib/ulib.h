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
 * Generate "random" number
 */
uint rand();


/*
 * This library expects:
 *
 * - Strings defined as uchar*
 * - Strings are finished with a 0
 * - New line is defined as "\n\r"
 */

/*
 * Send a character to the debug output
*/
void debugchar(uchar c);

/*
 * Send a complex string on the debug output
 * Supports:
 * %d (int), %u (uint), %x (uint), %s (uchar*),
 * %c (uchar), %U (ul_t) and %X (ul_t)
 */
void debugstr(uchar* format, ...);


/*
 * Send a character to the serial port
 */
void sputchar(uchar c);

/*
 * Send a formatted string to the serial port
 * Supports:
 * %d (int), %u (uint), %x (uint), %s (uchar*),
 * %c (uchar), %U (ul_t) and %X (ul_t)
 */
void sputstr(uchar* format, ...);

/*
 * Get char from serial port. Blocking function with time-out.
 * Returns 0 on timeout
 */
uchar sgetchar();


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
 * Get video mode. Text mode and graphics mode are supported
 * To be able to draw single pixels, use graphics mode.
 * If only text is needed, use text mode.
 * Graphics mode implements also a terminal emulation mode, so
 * text mode functions are all available
 */
#define VM_TEXT     0
#define VM_GRAPHICS 1
uint get_video_mode();

/*
 * Set video mode. See VM defines above
 */
void set_video_mode(uint mode);

/*
 * Get video mode screen size
 *
 * Mode can be SSM_CHARS (number of characters) for text and terminal emulation
 * modes and SSM_PIXELS (pixels) for graphics modes
 */
#define SSM_CHARS  0
#define SSM_PIXELS 1
void get_screen_size(uint mode, uint* width, uint* height);

/*
 * Clears the screen
 */
void clear_screen();

/*
 * Draw a pixel in graphics mode
 * color = byte, 256 color default VGA palette
 */
void set_pixel(uint x, uint y, uint color);

/*
 * Draw a char in graphics mode
 * color = byte, 256 color default VGA palette
 */
void draw_char(uint x, uint y, uint c, uint color);

/*
 * Draw a map in graphics mode
 * buff = array of bytes, one per pixel, 256 color default VGA palette
 */
void draw_map(uint x, uint y, uchar* buff, uint width, uint height);

/*
 * Send a character to the screen at cursor position
 */
void putchar(uchar c);

/*
 * Send a character to the screen at a given position and
 * specific attributes. See attribute flags above.
 *
 * Cursor will be relocated to col, row in the screen
 */
void putchar_attr(uint col, uint row, uchar c, uchar attr);

/*
 * Display complex string on the screen at cursor position
 * Supports:
 * %d (int), %u (uint), %x (uint), %s (uchar*),
 * %c (uchar), %U (ul_t) and %X (ul_t)
 */
void putstr(uchar* format, ...);


/*
 * Get cursor position
 */
void get_cursor_position(uint* col, uint* row);

/*
 * Set cursor position
 */
void set_cursor_position(uint col, uint row);

/*
 * Set cursor visibility
 * modes: HIDE_CURSOR, SHOW_CURSOR
 */
#define HIDE_CURSOR 0
#define SHOW_CURSOR 1
void set_show_cursor(uint mode);


/*
 * Special key codes
 *
 * getkey(...) function returns an uint
 * Alphanumeric and usual symbol key codes are their ASCII code.
 * Special key values are specified here.
 * Example:
 *
 * uint k = getkey(KM_NO_WAIT);
 * if(k == KEY_DEL) ...
 * if(k == 'a') ...
 */
#define KEY_BACKSPACE 0x0008
#define KEY_RETURN    0x000D
#define KEY_ESC       0x001B
#define KEY_DEL       0x5300
#define KEY_END       0x4F00
#define KEY_HOME      0x4700
#define KEY_INS       0x5200
#define KEY_PG_DN     0x5100
#define KEY_PG_UP     0x4900
#define KEY_PRT_SC    0x7200
#define KEY_TAB       0x0009

#define KEY_UP        0x4800
#define KEY_LEFT      0x4B00
#define KEY_RIGHT     0x4D00
#define KEY_DOWN      0x5000

#define KEY_F1        0x3B00
#define KEY_F2        0x3C00
#define KEY_F3        0x3D00
#define KEY_F4        0x3E00
#define KEY_F5        0x3F00
#define KEY_F6        0x4000
#define KEY_F7        0x4100
#define KEY_F8        0x4200
#define KEY_F9        0x4300
#define KEY_F10       0x4400
#define KEY_F11       0x8500
#define KEY_F12       0x8600

/*
 * Get key press in a char. Blocking function.
 * Special keys are ignored
 */
uchar getchar();

/*
 * Get pressed key code. If mode==KM_CLEAR_BUFFER clears the keyboard
 * buffer and returns 0. If mode==KM_NO_WAIT and keyboard buffer is
 * empty, returns 0. Otherwise, waits until there is a keypress in
 * buffer and returns its key code
 */
#define KM_NO_WAIT      0
#define KM_WAIT_KEY     1
#define KM_CLEAR_BUFFER 2
uint getkey(uint mode);

/*
 * Get a string from user. Returns when RETURN key is
 * pressed. Unused str characters are set to 0.
 * Returns number of elements in str
 */
uint getstr(uchar* str, uint max_count);

/*
 * Get mouse state
 * mode affects values returned in x and y and can be:
 * SSM_CHARS (text mode cursor, characters)
 * SSM_PIXELS (graphics mode, pixels)
 * b returns pressed buttons. Compare bits with MOUSE_X_BUTTON
 *
 */
#define MOUSE_LEFT_BUTTON 0x01
void get_mouse_state(uint mode, uint* x, uint* y, uint* b);


/*
 * Copy string src to dst
 */
uint strcpy(uchar* dst, uchar* src);

/*
 * Copy string src to dst without exceeding
 * dst_size elements in dst
 */
uint strcpy_s(uchar* dst, uchar* src, uint dst_size);

/*
 * Concatenate string src to dst
 */
uint strcat(uchar* dst, uchar* src);

/*
 * Concatenate string src to dst, without exceeding
 * dst_size elements in dst
 */
uint strcat_s(uchar* dst, uchar* src, uint dst_size);

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
 * Parse string uint. Can be hex "0x..." or dec
 * Returns:
 * - 0 if not possible
 * - uint value of src otherwise
 */
uint stou(uchar* src);

/*
 * Parse string and return 1 if it's an uint.
 * Returns:
 * - 0 if src is not an uint
 * - 1 if it is
 */
uint sisu(uchar* src);



/*
 * Copy size bytes from src to dst
 */
uint memcpy(uchar* dst, uchar* src, uint size);

/*
 * Set size bytes from dst to value
 */
uint memset(uchar* dst, uchar value, uint size);

/*
 * Compare size bytes from mem1 and mem2
 * Returns 0 if they are the same
 */
uint memcmp(uchar* mem1, uchar* mem2, uint size);


/*
 * Allocate size bytes of contiguous near memory.
 * The near memory is very limited and mostly used for code.
 * Use far memory (see below) when large amounts of memory are required.
 */
void* malloc(uint size);

/*
 * Free allocated memory
 */
void mfree(void* ptr);

/* FAR MEMORY
 *
 * This operating system uses a compact memory model: one code segment and
 * multiple data segments.
 *
 * The far pointers (or long pointers, lp_t) allow access to more than 1MB of
 * data outside the code segment, while near memory pointers do it only for
 * 64KB inside the code segment. Vars and malloc allocations are usually
 * allocated in near memory and can be used in a conventional way. Conversely,
 * far memory must be always managed by the kernel:
 *
 * - Since the compiler does not "understand" far pointers, they can't be
 *     used where near or conventional pointers are expected, like most
 *     system calls.
 * - Since the compiler does not "understand" far pointers, they can't be
 *     accessed by the usual way (pointer[offset] = value  : is not allowed).
 *     They can only be accessed using lmemcpy and lmemset functions.
 * - To copy far memory from/to near memory, use far memory functions and lp()
 *     function to cast near memory pointers to lp_t.
 *
 * Example:
 *
 * uchar cbuff[128];            // Conventional near memory
 * lp_t  xbuff = lmalloc(128);  // Long pointer, far memory
 *
 * // xbuff[0] = cbuff[0];  NOT ALLOWED
 * // cbuff[0] = xbuff[0];  NOT ALLOWED
 * // write(xbuff, ...); NOT ALLOWED. Instead, copy first to cbuff
 *
 * lmemcpy(xbuff, 0L, lp(cbuff), 0L, sizeof(buff)); // This is right
 * lmemcpy(lp(cbuff), 0L, xbuff, 0L, sizeof(buff)); // This is right
 *
 * lmfree(xbuff);
 */

/*
 * Convert pointer to lp_t
 */
lp_t lp(void* ptr);

/*
 * Copy size bytes from src[src_offs] to dst[dst_offs] (far memory)
 */
ul_t lmemcpy(lp_t dst, ul_t dst_offs, lp_t src, ul_t src_offs, ul_t size);

/*
 * Set size bytes from src to value (far memory)
 */
ul_t lmemset(lp_t dst, uchar value, ul_t size);


/*
 * Allocate size bytes of contiguous far memory
 */
lp_t lmalloc(ul_t size);
/*
 * Free allocated far memory
 */
void lmfree(lp_t ptr);


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

/* Special error codes */
#define ERROR_NOT_FOUND 0xFFFF
#define ERROR_EXISTS    0xFFFE
#define ERROR_IO        0xFFFD
#define ERROR_NO_SPACE  0xFFFC
#define ERROR_ANY       0xFFFB

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
  uint  id;        /* Disk id code */
  uint  fs_type;
  ul_t  fs_size;   /* MB */
  ul_t  disk_size; /* MB */
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

/*
 * Get system alive time in miliseconds
 */
ul_t get_timer();

/*
 * Wait an amount of miliseconds
 */
void wait(uint miliseconds);



#endif   /* _ULIB_H */
