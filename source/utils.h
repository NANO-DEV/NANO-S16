/*
 * Kernel utils
 */

#ifndef _UTILS_H
#define _UTILS_H

/*
 * BCD time date to int
 */
void BCD_time_to_time(struct TIME* td, char* BCDtime, char* BCDdate);

/*
 * Drive number to drive index
 */
int drive_to_index(int dr);
/*
 * Drive number to drive index
 */
int index_to_drive(int di);
/*
 * Drive number to string
 */
char* drive_to_string(int dr);
/*
 * String to drive number
 */
int string_to_drive(char* str);
/*
 * Check if string is drive string
 */
int string_is_drive(char* str);


#endif   /* _UTILS_H */
