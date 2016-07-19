/*
 * Echo program
 */

#include "../clib/clib.h"

char* argv;

/*
 * Entry point
 */
int entry(void)
{
    int i = 0;

    /* Remove initial spaces */
    while(argv[i] == ' ') {
        i++;
    }

    /* Remove first command name */
    while(argv[i] != ' ' && argv[i] != 0) {
        i++;
    }

    /* Remove spaces until string */
    while(argv[i] == ' ') {
        i++;
    }

    /* Print string */
    printf("%s\n\r", &argv[i]);
    
    return 0;
}
