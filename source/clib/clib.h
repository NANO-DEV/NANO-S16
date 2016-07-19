/*
 * C-Lib
 */

#ifndef _CLIB_H
#define _CLIB_H

/*
 * Include definition file
 */
#include "def.h"


/*
 * Integer division modulus
 */
int mod(int D, int d);
/*
 * Get HIGH byte
 */
char getHI(int c);
/*
 * Get LOW byte
 */
char getLO(int c);


/*
 * Send a character to the serial port
 */
void sputchar(char c);
/*
 * Send a string to the serial port
 */
void sputs(char *str);
/*
 * Send a complex string to the serial port
 */
int sprintf(char *format, ...);



/*
 * Clears the screen
 */
void clear_screen(void);
/*
 * Send a character to the screen
 */
void putchar(char c);
/*
 * Display string on the screen
 */
void puts(char *str);
/*
 * Display complex string on the screen
 */
int printf(char *format, ...);
/*
 * Send a character to the screen with attr
 */
void putchar_attr(int x, int y, char c, char attr);
/*
 * Set cursor position
 */
void set_cursor_position(int x, int y);



/*
 * Get key press in a char
 */
char getchar(void);
/*
 * Get key press
 */
int getkey(void);
/*
 * Get a string from user
 */
void gets(char* str);



/*
 * Copy string src to dest
 */
int strcpy(char *dest, char *src);
/*
 * Copy n elements from string src to dest
 */
int strncpy(char *dest, char *src, int n);
/*
 * Concatenate string src to dest
 */
int strcat(char *dest, char *src);
/*
 * Get string length
 */
int strlen(char* str);
/*
 * Compare strings
 */
int strcmp(char* str1, char* str2);
/*
 * Compare n elements of strings
 */
int strncmp(char* str1, char* str2, int n);
/*
 * Tokenize string
 */
 int strtok(char* spl, int na, int ss, int* ns, char* str);



/*
 * Copy size bytes from src to dest
 */
int memcpy(char *dest, char *src, int size);
/*
 * Set size bytes from src to value
 */
int memset(char *dest, char value, int size);



/*
 * Allocate memory block
 */
void* malloc(int size);
/*
 * Free memory block
 */
void free(void* ptr);


/*
 * Get filesystem info
 */
int get_fsinfo(int* n, struct FS_INFO** info);
/*
 * Get CWD
 */
int getCWD(struct FS_PATH* cwd);
/*
 * Set CWD
 */
int setCWD(struct FS_PATH* cwd);
/*
 * Get dir entry
 */
int get_direntry(char* dir, int n, struct FS_DIRENTRY* direntry);
/*
 * Get entry
 */
int get_entry(char* file, struct FS_DIRENTRY* entry);
/*
 * Create Dir
 */
int create_dir(char* dir);
/*
 * Set file size
 */
int set_file_size(char* file, int size);
/*
 * Delete
 */
int delete(char* file);
/*
 * Rename
 */
int rename(char* file, char* new_name);
/*
 * Read file
 */
int read_file(char* file, int offset, int buff_size, char* buff);
/*
 * Write file
 */
int write_file(char* file, int offset, int buff_size, char* buff);


/*
 * Get system date and time
 */
void time(struct TIME &time);


/*
 * Perform system call
 */
int syscall(int service, void* param);



#endif   /* _CLIB_H */
