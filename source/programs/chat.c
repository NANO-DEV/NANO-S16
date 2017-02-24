/*
* NANO-Chat
* Version 0.2
* By Jonatan Linares.
*/

#include "types.h"
#include "ulib/ulib.h"

#define NICK_LEN 50
#define BUFFER_LEN 250

uint main(uint argc, uchar* argv[]) {
  uchar* nick = malloc(NICK_LEN + 1);
  uchar* oBuffer = malloc(BUFFER_LEN + 1);
  uchar* iBuffer = malloc(BUFFER_LEN + 1);
  uchar data;
  uchar* dataStr = malloc(2);
  uint len;
  uint rKey;
  uint key;
  uint i;
  uint current_line = 7;

  iBuffer[0] = 0;
  oBuffer[0] = 0;

  clear_screen();
  putstr("NANO-Chat\n\r");
  putstr("v0.2\n\r");
  putstr("---------\n\r");

  putstr("*** What's your name?\n\r");
  getstr(nick, NICK_LEN);

  putstr("*** Joined! (ESC to exit)\n\r\n\r");

  while(1) {
    // Receive

    data = sgetchar();

    if (data == 59) {// Entire (Ends with ";")
      if (oBuffer[0] != 0) {
        putstr("\n\r");
        current_line++;
      }

      putstr("--- %s\n\r", iBuffer);
      current_line++;
      iBuffer[0] = 0;

      putstr("%s", oBuffer);
    }
    else if (data != 0) {// Fragment
      len = strlen(iBuffer);

      iBuffer[len] = data;
      iBuffer[len + 1] = 0;
    }

    // Send
    
    for(i = 0; i < 32000; i++) {
      rKey = getkey(NO_WAIT);// oBuffer, BUFFER_LEN
      key = getLO(rKey);

      if (key == KEY_LO_ESC) {// Exit
        putstr("\n\rThanks for chatting. Bye!\n\r\n\r");

        return 1;
      }
      else if (key == KEY_LO_RETURN) {// Entire ("Enter" pressed)
        if (oBuffer[0] != 0) {
          putstr("\n\r");
          current_line++;

          sputstr("%s: %s;", nick, oBuffer);
          oBuffer[0] = 0;
        }
      }
      else if (key == KEY_LO_BACKSPACE) {// Backspace
        len = strlen(oBuffer);

        if (len > 0) {
          set_cursor_position(len - 1, current_line);
          putchar(32);// Space
          set_cursor_position(len - 1, current_line);

          oBuffer[len - 1] = 0;
        }
      }
      else if (key != 0) {// Fragment
        putchar(key);

        len = strlen(oBuffer);

        oBuffer[len] = key;
        oBuffer[len + 1] = 0;
      }
    }
  }

  return 0;
}
