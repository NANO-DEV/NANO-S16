/*
* NANO-Browser
* By Jonatan Linares.
*/

#include "types.h"
#include "ulib/ulib.h"

#define ADDRESS_LEN 500

uint main(uint argc, uchar* argv[]) {
  uchar data;
  uint i;
  uchar* address = malloc(ADDRESS_LEN + 1);

  clear_screen();
  putstr("NANO-Browser\n\r");
  putstr("v0.1\n\r");
  putstr("---------\n\r");

  while(1) {
    i = 0;

    putstr("\n\r\n\r*** Website Address? (\"Q\" to exit)\n\r");
    putstr("http://");
    getstr(address, ADDRESS_LEN);
    clear_screen();

    // Exit
    if (address[0] == 81) {
      return 1;
    }

    // Get content
    sputstr("http://%s;", address);
    while(i < 5) {
      data = sgetchar();

      if (data == 0) {
        i++;
      }
      else {
        i = 0;
      }

      if (data != 0) {
        putchar(data);
      }
    }
  }

  return 0;
}
