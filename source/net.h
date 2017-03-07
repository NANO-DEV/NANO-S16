/*
 * Network functionality
 */

#ifndef _NET_H
#define _NET_H

extern uchar local_ip[4];
extern uchar local_gate[4];

/*
 * Convert string to IP
 */
void str_to_ip(uchar* ip, uchar* str);

/*
 * Initialize network
 */
void net_init();

/*
 * Test network
 */
void net_test();

/*
 * Send buffer to dstIP
 */
uint net_send(uchar* dstIP, uchar* buff, uint len);

/*
 * Receive data
 */
uint net_recv(uchar* srcIP, uchar* buff, uint buff_size);

#endif  /* _NET_H */
