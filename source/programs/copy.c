/*
 * Copy program
 */

#include "../clib/clib.h"

char* argv;

/*
 * Entry point
 */
#define MAX_ARG   4
#define ARG_SIZE 64
int entry(void)
{
    char arg[MAX_ARG][ARG_SIZE];
    int a = 0;
    int result = 0;

    /* Parse args */
    strtok(arg, MAX_ARG, ARG_SIZE, &a, argv);

    /* Check arg number */
    if(a != 2) {
        printf("Usage: copy <src> <dst>\n\r");
        result = 1;
    } else {
        /* Find entry */
        struct FS_DIRENTRY direntry;
        int result = get_entry(arg[1], &direntry);
        if(result == 0) {
            /* Load source file */
            char* buffer = malloc(direntry.size);
            if(buffer != 0) {
                result = read_file(arg[1], 0, direntry.size, buffer);
                /* Write dest file */
                if(result != 0) {
                    printf("Can't load source file (%s)\n\r", arg[1]);
                } else {
                    result = write_file(arg[2], 0, direntry.size, buffer);
                    if(result != 0) {
                        printf("Error writting file\n\r");
                    }
                    free(buffer);
                }
            } else {
                printf("Not enough memory\n\r");
                result = 1;
            }
        }
    }
    return result;
}
