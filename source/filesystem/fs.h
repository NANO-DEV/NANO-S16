/*
 * Filesystem
 */

#ifndef _FS_H
#define _FS_H

/*
 * Init filesystem
 */
int fs_init();
/*
 * Get filesystem info
 */
int fs_get_info(int* n, struct CFS_INFO** info);
/*
 * Set current drive
 */
int fs_set_drive(int drive);
/*
 * Get CWD
 */
int fs_getCWD(struct CFS_PATH* cwd);
/*
 * Set CWD
 */
int fs_setCWD(struct CFS_PATH* cwd);
/*
 * Get dir entry
 */
int fs_get_direntry(char* dir, int n, struct FS_DIRENTRY* direntry);
/*
 * Get entry
 */
int fs_get_entry(char* file struct FS_DIRENTRY* entry);
/*
 * Create Dir
 */
int fs_create_dir(char* dir);
/*
 * Set file size
 */
int fs_set_file_size(char* file, int size);
/*
 * Delete
 */
int fs_delete(char* file);
/*
 * Rename
 */
int fs_rename(char* file, char* new_name);
/*
 * Read file
 */
int fs_read_file(char* file, int offset, int buff_size, char* buff);
/*
 * Write file
 */
int fs_write_file(char* file, int offset, int buff_size, char* buff);
/*
 * Sys drive
 */
int fs_sys_drive(int dst, int src);


#endif   /* _FS_H */
