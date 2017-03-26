/*
 * User program: Basic net interface
 */

#include "types.h"
#include "ulib/ulib.h"

/*
 * Program entry point
 */
uint main(uint argc, uchar* argv[])
{
  uint result = 0;

  /* Send or receive through network */
  if(argc == 2 && strcmp(argv[1], "recv") == 0) {
    uchar buff[64];
    uint8_t src_ip[4];

    /* Receive in buffer */
    result = recv(src_ip, buff, sizeof(buff));

    /* If result == 0, nothing has been received */
    if(result == 0) {
      putstr("Buffer is empty\n\r");

    /* Else, result contains number of received bytes */
    } else {
      /* Add a 0 after received data */
      buff[result] = 0;

      /* Show source address and contents */
      putstr("Received %s from %u.%u.%u.%u\n\r", buff,
        src_ip[0], src_ip[1], src_ip[2], src_ip[3]);
    }
  } else if(argc == 4 && strcmp(argv[1], "send") == 0) {
    uint8_t dst_ip[4];

    /* Parse IP from string */
    str_to_ip(dst_ip, argv[2]);

    /* Send */
    result = send(dst_ip, argv[3], strlen(argv[3]));

    /* If result == 0, then data was sent */
    if(result == 0) {
      putstr("Sent %s to %u.%u.%u.%u\n\r", argv[3],
        dst_ip[0], dst_ip[1], dst_ip[2], dst_ip[3]);
    } else {
      /* Else, failed */
      putstr("Failed to send\n\r");
    }

  /* Live node to node chat */
  } else if(argc == 3 && strcmp(argv[1], "chat") == 0) {
    uint8_t dst_ip[4];
    uchar send_buff[256];

    /* Initialize buffer */
    memset(send_buff, 0, sizeof(send_buff));

    /* Parse IP from string */
    str_to_ip(dst_ip, argv[2]);

    /* Show banner */
    putstr("Chat with %u.%u.%u.%u. Press ESC to exit\n\r",
      dst_ip[0], dst_ip[1], dst_ip[2], dst_ip[3]);

    /* Main loop */
    while(1) {
      uint k;
      uchar recv_buff[256];
      uint8_t recv_ip[4];

      /* Receive in buffer */
      result = recv(recv_ip, recv_buff, sizeof(recv_buff));

      /* If received something and source is desired source... */
      if(result != 0 &&
        memcmp(recv_ip, dst_ip, sizeof(dst_ip)) == 0) {

        /* Show source address and contents */
        putstr("\r%d.%d.%d.%d: %s\n\r",
          dst_ip[0], dst_ip[1], dst_ip[2], dst_ip[3],
          recv_buff);
      }

      /* Process keyboard input */
      k = getkey(KM_NO_WAIT);

      /* Exit if key ESC */
      if(k == KEY_ESC) {
        break;
      }

      /* Send if key RETURN */
      else if(k == KEY_RETURN) {
        result = send(dst_ip, send_buff, strlen(send_buff));

        /* If result == 0, then data was sent */
        if(result == 0) {
          putstr("\rlocal: %s\n\r", send_buff);
        } else {
          /* Else, failed */
          putstr("\rFailed to send\n\r");
        }

        /* Reset buffer */
        memset(send_buff, 0, sizeof(send_buff));
      }

      /* Delete last char */
      else if((k == KEY_BACKSPACE || k == KEY_DEL) &&
        strlen(send_buff) > 0) {
        send_buff[strlen(send_buff)-1] = 0;
        putstr("\r \r%s", send_buff);
      }

      /* Append char */
      else if(k >= ' ' && k <= '}') {
        send_buff[min(sizeof(send_buff), strlen(send_buff))] = getLO(k);
        putstr("\r%s", send_buff);
      }
    }

    /* Finished */
    putstr("-ESC-\n\r");

  } else {
    putstr("usage: %s <send <dst_ip> <word> | recv | chat <dst_ip>>\n\r", argv[0]);
  }

  return result;
}
