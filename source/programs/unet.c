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
      putstr("Received %s from %d.%d.%d.%d\n\r", buff,
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
      putstr("Sent %s to %d.%d.%d.%d\n\r", argv[3],
        dst_ip[0], dst_ip[1], dst_ip[2], dst_ip[3]);
    } else {
      /* Else, failed */
      putstr("Failed to send\n\r");
    }
  } else {
    putstr("usage: %s <send <dst_ip> <word> | recv>\n\r", argv[0]);
  }

  return result;
}
