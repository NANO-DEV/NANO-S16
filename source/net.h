/*
 * Network functionality
 */

#ifndef _NET_H
#define _NET_H


/* Default value. If hardware is not compatible
 * network will be automatically disabled */
extern uint network_enabled;

/* About IP format
 *
 * Unless something different is specified:
 * uint8_t* ip   are arrays of IP_LEN bytes with a parsed IP
 * uchar* ip     are strings containing an unparsed IP
 */
#define IP_LEN 4 /* Number of bytes of an IP */

extern uint8_t local_ip[IP_LEN];
extern uint8_t local_gate[IP_LEN];

/*
 * Convert string to IP
 */
void str_to_ip(uint8_t* ip, uchar* str);

/*
 * Initialize network
 */
void net_init();

/*
 * Send buffer to dst_ip
 */
uint net_send(uint8_t* dst_ip, uchar* buff, uint len);

/*
 * Get and remove from buffer received data.
 * src_ip and buff are filled by the function
 */
uint net_recv(uint8_t* src_ip, uchar* buff, uint buff_size);

#endif  /* _NET_H */
