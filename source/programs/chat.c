/*
* NANO-Chat
* By Jonatan Linares.
*/

#include "types.h"
#include "ulib/ulib.h"

#define NICK_LEN 50
#define BUFFER_LEN 1000

uint main(uint argc, uchar* argv[]) {
  uchar* nick = malloc(NICK_LEN + 1);
  uchar* buffer = malloc(BUFFER_LEN + 1);
  uchar data;
  uint len;

  putstr("NANO-Chat\n\r");
  putstr("v0.1\n\r");
  putstr("---------\n\r");

  putstr("*** What's your name?\n\r");
  getstr(nick, NICK_LEN);

  putstr("*** Joined!\n\r");

  while(1) {
    data = sgetchar();

    if (data == ";") {// Entire
      putstr("%s", buffer);
      buffer[0] = "\0";
    }
    else {// Fragment
      len = strlen(buffer);

      buffer[len] = data;
      buffer[len + 1] = "\0";
    }
  }

  // sputstr("we");

  return 0;
}
