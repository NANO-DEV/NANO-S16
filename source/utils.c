/*
 * Kernel utils
 */

#include "kernel.h"
#include "clib/clib.h"
#include "utils.h"

/*
 * BCD to int
 */
static int BCD_to_int(char BCD)
{
    int h = (BCD >> 4) * 10;
    return h + (BCD & 0xF);
}

/*
 * BCD time date to int
 */
void BCD_time_to_time(struct TIME* td, char* BCDtime, char* BCDdate)
{
    td->hour   = BCD_to_int(BCDtime[0]);
    td->minute = BCD_to_int(BCDtime[1]);
    td->second = BCD_to_int(BCDtime[2]);
    td->year   = BCD_to_int(BCDdate[0]);
    td->month  = BCD_to_int(BCDdate[1]);
    td->day    = BCD_to_int(BCDdate[2]);
}

/*
 * Drive number to drive index
 */
int drive_to_index(int dr)
{
    if(dr >= 0x80) {
        return dr - 0x80 + 2;
    }

    return dr;
}

/*
 * Drive index to drive number
 */
int index_to_drive(int di)
{
    if(di >= 2) {
        return di + 0x80 - 2;
    }

    return di;
}

/*
 * Drive number to string
 */
char* drive_to_string(int dr)
{
    switch(dr) {
    case 0x00:
        return "fd0";
    case 0x01:
        return "fd1";
    case 0x80:
        return "hd0";
    case 0x81:
        return "hd1";
    };
    return "unk";
}

/*
 * String to drive number
 */
int string_to_drive(char* str)
{
    if(strcmp(str, "fd0") == 0) {
        return 0x00;
    } else if(strcmp(str, "fd1") == 0) {
        return 0x01;
    } else if(strcmp(str, "hd0") == 0) {
        return 0x80;
    } else if(strcmp(str, "hd1") == 0) {
        return 0x81;
    }
    return 2;
}

/*
 * Check if string is drive string
 */
int string_is_drive(char* str)
{
    int result = 0;
    int i = 0;

    for(i=0; i<K_MAX_DRIVE; i++) {
        int dr = index_to_drive(i);
        if(strcmp(str, drive_to_string(dr)) == 0) {
            return 1;
        }
    }
    return result;
}
