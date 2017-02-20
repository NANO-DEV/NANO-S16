/*
* Hello for NANO.
* By Jonatan Linares.
*/

#include "types.h"
#include "ulib/ulib.h"

#define NAME_LEN 50

uint main(uint argc, uchar* argv[]) {
  uchar* name = malloc(NAME_LEN + 1);

  putstr("What's your name?\n\r");

  getstr(name, NAME_LEN);

  putstr("Hello %s! Nice to meet you.\n\r", name);

  return 0;
}
