/*
 * Command line interface
 */

#ifndef _CLI_H
#define _CLI_H

// Run the integrated CLI
// This function should never return
void cli();

// Execute script file
void cli_exec_file(char* path);

#endif // _CLI_H