/*
* NANO-Chat
* By Jonatan Linares.
*/

#include "types.h"
#include "ulib/ulib.h"

#define NICK_LEN 50
#define BUFFER_LEN 10000

uint main(uint argc, uchar* argv[]) {
  uchar* nick = malloc(NICK_LEN + 1);
  uchar* oBuffer = malloc(BUFFER_LEN + 1);
  uchar* iBuffer = malloc(BUFFER_LEN + 1);
  uchar data;
  uchar* dataStr = malloc(2);
  uint len;
  uint i;

  iBuffer[0] = 0;
  oBuffer[0] = 0;

  putstr("NANO-Chat\n\r");
  putstr("v0.1\n\r");
  putstr("---------\n\r");

  putstr("*** What's your name?\n\r");
  getstr(nick, NICK_LEN);

  putstr("*** Joined! After the 10 seconds of waitting, you can send your message (Or press ENTER / Q)\n\r");

  while(1) {
    i = 0;
    iBuffer[0] = 0;

    // Receive
    while(i < 10) {
      data = sgetchar();

      if (data == 0) {
        i++;
      }
      else {
        i = 0;
      }

      if (data == 59) {// Entire (Ends with ";")
        putstr("\n\r%s", iBuffer);
        iBuffer[0] = 0;
      }
      else {// Fragment
        len = strlen(iBuffer);

        if (data != 0) {
          iBuffer[len] = data;
          iBuffer[len + 1] = 0;
        }
      }
    }
    putstr("\n\r");

    // Send
    putchar(43);
    getstr(oBuffer, BUFFER_LEN);
    if (oBuffer[0] == 81) {// Exit
      return 1;
    }
    else if (strlen(oBuffer) > 0) {
      sputstr("%s: %s;", nick, oBuffer);
    }
    oBuffer[0] = 0;
  }

  return 0;
}
