#include "types.h"
#include "hw86.h"
#include "ulib/ulib.h"
#include "net.h"

/* Network controller
 * Assumes NE2000 compatible nic
 * http://www.ethernut.de/pdf/8019asds.pdf */


/* Install network IRQ handler */
extern void install_net_IRQ_handler();

/* Byte swap */
#define bswap_16(value) \
((((value) & 0xff) << 8) | ((value) >> 8))

/* Test data */
uchar test_data[42] =
{'0', '1', '2', '3', '4', '5', '6', '7',
 '8', '9', 'A', 'B', 'C', 'D', 'E', 'F',
 '0', '1', '2', '3', '4', '5', '6', '7',
 '8', '9', 'A', 'B', 'C', 'D', 'E', 'F',
 '0', '1', '2', '3', '4', '5', '6', '7',
 'E', 'P' };

/* Registers */
#define CR		  0x00 /* Command register */
/* 7-6:PS1-PS0 5-3:RD2-0 2:TXP 1:STA 0:STP */

/* page 0, read */
#define CLDA0		0x01
#define CLDA1		0x02
#define BNRY		0x03 /* Boundary rw */
#define TSR		  0x04 /* Transmit status */
#define NCR		  0x05 /* Collision counter */
#define FIFO		0x06
#define ISR		  0x07 /* Interrupt status rw */
#define CRDA0		0x08
#define CRDA1		0x09
#define RSR		  0x0C
#define CNTR0		0x0D /* Error counters */
#define CNTR1		0x0E
#define CNTR2		0x0F

/* page 0 write */
#define PSTART	0x01 /* Page start read page 2 */
#define PSTOP		0x02 /* Page stop read page 2 */
#define TPSR		0x04 /* Transmit page start read page 2 */
#define TBCR0		0x05
#define TBCR1		0x06
#define RSAR0		0x08 /* Remote start address */
#define RSAR1		0x09
#define RBCR0		0x0A /* Remote byte count */
#define RBCR1		0x0B
#define RCR		  0x0C /* Receive config read page 2 */
#define TCR		  0x0D /* Transmit config read page 2 */
#define DCR		  0x0E /* Data config read page 2 */
#define IMR		  0x0F /* Interrupt mask read page 2 */

/* page 1 read/write */
#define PAR0		0x01 /* Physical address */
#define PAR1		0x02
#define PAR2		0x03
#define PAR3		0x04
#define PAR4		0x05
#define PAR5		0x06
#define CURR		0x07 /* Current page */
#define MAR0		0x08 /* Multicast address */
#define MAR1		0x09
#define MAR2		0x0A
#define MAR3		0x0B
#define MAR4		0x0C
#define MAR5		0x0D
#define MAR6		0x0E
#define MAR7		0x0F

#define DATA		0x10
#define RESET		0x1F

#define NE2K_STAT_RX    0x0001 /* packet received */
#define NE2K_STAT_TX    0x0002 /* packet sent */

static uint rx_next = 0x47;

static uint base = 0x300; /* Base device port */
static uchar irq = 9; /* IRQ */
static uchar local_mac[6]; /* Get from network card */
uchar local_ip[4] = {192,168,1,2};
uchar local_gate[4] = {192,168,1,1};
static uchar local_net[4] = {255,255,255,0};

/* Ethernet related */
#define ETH_ADDR_LEN	6

struct ethhdr {
	uint8_t  dst[ETH_ADDR_LEN];
	uint8_t  src[ETH_ADDR_LEN];
	uint16_t type;
	uint8_t  data[0];	/* size 46-1500 */
};

/* Values of ethhdr->eh_type */
#define ETH_TYPE_ARP	0x0806
#define ETH_TYPE_IP	  0x0800

#define ETH_HDR_LEN	  14

#define ETH_MTU		    1500
#define ETH_VLAN_LEN	4
#define ETH_CRC_LEN	  4

#define ETH_PKT_MAX_LEN	(ETH_HDR_LEN+ETH_VLAN_LEN+ETH_MTU)

/* ARP related */
#define	ARP_HADDR_LEN	ETH_ADDR_LEN	/* Size of Ethernet MAC address	*/
#define	ARP_PADDR_LEN	4		/* Size of IP address	*/

struct arphdr {
	uint16_t hrd; 	/* format of hardware address	*/
	uint16_t pro; 	/* format of protocol address	*/
	uint8_t  hln; 	/* length of hardware address	*/
	uint8_t  pln; 	/* length of protocol address	*/
	uint16_t op;  	/* arp/rarp operation	*/
	uint8_t  sha[ARP_HADDR_LEN];
	uint8_t  spa[ARP_PADDR_LEN];
	uint8_t  dha[ARP_HADDR_LEN];
	uint8_t  dpa[ARP_PADDR_LEN];
};

/* Values of arphdr->ah_hrd */
#define ARP_HTYPE_ETHER	1	/* Ethernet hardware type	*/

/* Values of arphdr->ah_pro */
#define ARP_PTYPE_IP		0x0800	/* IP protocol type */
#define ARP_PTYPE_ARP		0x0806	/* ARP protocol type */

/* Values of arphdr->ah_op */
#define ARP_OP_REQUEST	1	/* Request op code */
#define ARP_OP_REPLY		2	/* Reply op code */


/* IP Protocol */
#define IP_PROTOCOL_ICMP  1
#define IP_PROTOCOL_TCP   6
#define IP_PROTOCOL_UDP   17


/* IPv4 Header */
struct IPv4hdr {
  uint8_t  verIhl;
  uint8_t  tos;
  uint16_t len;
  uint16_t id;
  uint16_t offset;
  uint8_t  ttl;
  uint8_t  protocol;
  uint16_t checksum;
  uint8_t  src[4];
  uint8_t  dst[4];
};

/* UDP header */
struct UDPhdr {
  uint16_t srcPort;
  uint16_t dstPort;
  uint16_t len;
  uint16_t checksum;
};

/* ARP table to hold hw-p entries */
#define ARP_TABLE_LEN 8
struct ARP_TABLE {
  uchar p[ARP_PADDR_LEN];
  uchar hw[ARP_HADDR_LEN];
} arp_table[ARP_TABLE_LEN];

/* Given a p address, provide hw address */
uchar* find_hw_in_table(uchar* p)
{
  uint i = 0;

  /* Local ip -> local mac */
  if(memcmp(p, local_ip) == 0) {
    return local_mac;
  }

  /* Search in the real table */
  for(i=0; i<ARP_TABLE_LEN; i++) {
    uint j = 0;
    for(j=0; j<ARP_PADDR_LEN; j++) {
      if(arp_table[i].p[j] != p[j]) {
        break;
      }
    }
    if(j==ARP_PADDR_LEN) {
      return arp_table[i].hw;
    }
  }
  return 0;
}

/* Buffers to sebd/receive packets */
uchar tmp_buff[512];
uchar rcv_buff[512];
uchar rcv_addr[4];
uint  rcv_port = 0;
uint  rcv_size = 0;
uchar snd_buff[512];

/* For now, lets save only what is received in this port */
#define NSUDP_PROTO_PORT 8086


 /* Convert string to IP */
void str_to_ip(uchar* ip, uchar* str)
{
  uint i = 0;
  uchar tok_str[32];
  uchar* tok = tok_str;
  uchar* nexttok = tok;
  strcpy_s(tok_str, str, sizeof(tok_str));

  /* Tokenize */
  while(*tok && *nexttok && i<4) {
    tok = strtok(tok, &nexttok, '.');
    if(*tok) {
      ip[i++] = stou(tok);
    }
    tok = nexttok;
  }
}

/* Keep checksum 16-bits */
uint16_t net_checksum_final(uint sum)
{
  uint16_t temp;
  sum = (sum&0xffff) + (sum>>16);
  sum += (sum>>16);

  temp = ~sum;
  return ((temp&0x00ff)<<8) | ((temp&0xff00)>>8);
}

/* Checksum accumulation function */
uint32_t net_checksum_acc(uint8_t* data, uint32_t len)
{
  uint32_t sum = 0;
  uint16_t* p = (uint16_t*)data;

  while(len > 1) {
    sum += *p++;
    len -= 2;
  }

  if(len) {
    sum += *(uint8_t*)p;
  }

  return sum;
}

/* Main checksum function */
uint16_t net_checksum(uint8_t* data, uint len)
{
  uint32_t sum = net_checksum_acc(data, (uint32_t)len);
  return net_checksum_final(sum);
}

/* This helps calculating CRC */
uint32_t poly8_lookup[256] =
{
 0x00000000L, 0x77073096L, 0xEE0E612CL, 0x990951BAL,
 0x076DC419L, 0x706AF48FL, 0xE963A535L, 0x9E6495A3L,
 0x0EDB8832L, 0x79DCB8A4L, 0xE0D5E91EL, 0x97D2D988L,
 0x09B64C2BL, 0x7EB17CBDL, 0xE7B82D07L, 0x90BF1D91L,
 0x1DB71064L, 0x6AB020F2L, 0xF3B97148L, 0x84BE41DEL,
 0x1ADAD47DL, 0x6DDDE4EBL, 0xF4D4B551L, 0x83D385C7L,
 0x136C9856L, 0x646BA8C0L, 0xFD62F97AL, 0x8A65C9ECL,
 0x14015C4FL, 0x63066CD9L, 0xFA0F3D63L, 0x8D080DF5L,
 0x3B6E20C8L, 0x4C69105EL, 0xD56041E4L, 0xA2677172L,
 0x3C03E4D1L, 0x4B04D447L, 0xD20D85FDL, 0xA50AB56BL,
 0x35B5A8FAL, 0x42B2986CL, 0xDBBBC9D6L, 0xACBCF940L,
 0x32D86CE3L, 0x45DF5C75L, 0xDCD60DCFL, 0xABD13D59L,
 0x26D930ACL, 0x51DE003AL, 0xC8D75180L, 0xBFD06116L,
 0x21B4F4B5L, 0x56B3C423L, 0xCFBA9599L, 0xB8BDA50FL,
 0x2802B89EL, 0x5F058808L, 0xC60CD9B2L, 0xB10BE924L,
 0x2F6F7C87L, 0x58684C11L, 0xC1611DABL, 0xB6662D3DL,
 0x76DC4190L, 0x01DB7106L, 0x98D220BCL, 0xEFD5102AL,
 0x71B18589L, 0x06B6B51FL, 0x9FBFE4A5L, 0xE8B8D433L,
 0x7807C9A2L, 0x0F00F934L, 0x9609A88EL, 0xE10E9818L,
 0x7F6A0DBBL, 0x086D3D2DL, 0x91646C97L, 0xE6635C01L,
 0x6B6B51F4L, 0x1C6C6162L, 0x856530D8L, 0xF262004EL,
 0x6C0695EDL, 0x1B01A57BL, 0x8208F4C1L, 0xF50FC457L,
 0x65B0D9C6L, 0x12B7E950L, 0x8BBEB8EAL, 0xFCB9887CL,
 0x62DD1DDFL, 0x15DA2D49L, 0x8CD37CF3L, 0xFBD44C65L,
 0x4DB26158L, 0x3AB551CEL, 0xA3BC0074L, 0xD4BB30E2L,
 0x4ADFA541L, 0x3DD895D7L, 0xA4D1C46DL, 0xD3D6F4FBL,
 0x4369E96AL, 0x346ED9FCL, 0xAD678846L, 0xDA60B8D0L,
 0x44042D73L, 0x33031DE5L, 0xAA0A4C5FL, 0xDD0D7CC9L,
 0x5005713CL, 0x270241AAL, 0xBE0B1010L, 0xC90C2086L,
 0x5768B525L, 0x206F85B3L, 0xB966D409L, 0xCE61E49FL,
 0x5EDEF90EL, 0x29D9C998L, 0xB0D09822L, 0xC7D7A8B4L,
 0x59B33D17L, 0x2EB40D81L, 0xB7BD5C3BL, 0xC0BA6CADL,
 0xEDB88320L, 0x9ABFB3B6L, 0x03B6E20CL, 0x74B1D29AL,
 0xEAD54739L, 0x9DD277AFL, 0x04DB2615L, 0x73DC1683L,
 0xE3630B12L, 0x94643B84L, 0x0D6D6A3EL, 0x7A6A5AA8L,
 0xE40ECF0BL, 0x9309FF9DL, 0x0A00AE27L, 0x7D079EB1L,
 0xF00F9344L, 0x8708A3D2L, 0x1E01F268L, 0x6906C2FEL,
 0xF762575DL, 0x806567CBL, 0x196C3671L, 0x6E6B06E7L,
 0xFED41B76L, 0x89D32BE0L, 0x10DA7A5AL, 0x67DD4ACCL,
 0xF9B9DF6FL, 0x8EBEEFF9L, 0x17B7BE43L, 0x60B08ED5L,
 0xD6D6A3E8L, 0xA1D1937EL, 0x38D8C2C4L, 0x4FDFF252L,
 0xD1BB67F1L, 0xA6BC5767L, 0x3FB506DDL, 0x48B2364BL,
 0xD80D2BDAL, 0xAF0A1B4CL, 0x36034AF6L, 0x41047A60L,
 0xDF60EFC3L, 0xA867DF55L, 0x316E8EEFL, 0x4669BE79L,
 0xCB61B38CL, 0xBC66831AL, 0x256FD2A0L, 0x5268E236L,
 0xCC0C7795L, 0xBB0B4703L, 0x220216B9L, 0x5505262FL,
 0xC5BA3BBEL, 0xB2BD0B28L, 0x2BB45A92L, 0x5CB36A04L,
 0xC2D7FFA7L, 0xB5D0CF31L, 0x2CD99E8BL, 0x5BDEAE1DL,
 0x9B64C2B0L, 0xEC63F226L, 0x756AA39CL, 0x026D930AL,
 0x9C0906A9L, 0xEB0E363FL, 0x72076785L, 0x05005713L,
 0x95BF4A82L, 0xE2B87A14L, 0x7BB12BAEL, 0x0CB61B38L,
 0x92D28E9BL, 0xE5D5BE0DL, 0x7CDCEFB7L, 0x0BDBDF21L,
 0x86D3D2D4L, 0xF1D4E242L, 0x68DDB3F8L, 0x1FDA836EL,
 0x81BE16CDL, 0xF6B9265BL, 0x6FB077E1L, 0x18B74777L,
 0x88085AE6L, 0xFF0F6A70L, 0x66063BCAL, 0x11010B5CL,
 0x8F659EFFL, 0xF862AE69L, 0x616BFFD3L, 0x166CCF45L,
 0xA00AE278L, 0xD70DD2EEL, 0x4E048354L, 0x3903B3C2L,
 0xA7672661L, 0xD06016F7L, 0x4969474DL, 0x3E6E77DBL,
 0xAED16A4AL, 0xD9D65ADCL, 0x40DF0B66L, 0x37D83BF0L,
 0xA9BCAE53L, 0xDEBB9EC5L, 0x47B2CF7FL, 0x30B5FFE9L,
 0xBDBDF21CL, 0xCABAC28AL, 0x53B39330L, 0x24B4A3A6L,
 0xBAD03605L, 0xCDD70693L, 0x54DE5729L, 0x23D967BFL,
 0xB3667A2EL, 0xC4614AB8L, 0x5D681B02L, 0x2A6F2B94L,
 0xB40BBE37L, 0xC30C8EA1L, 0x5A05DF1BL, 0x2D02EF8DL
};

/* bcc fails to provide this func */
uint32_t xor32(uint32_t a, uint32_t b)
{
  uint i;
  uint32_t r;
  uchar* pa = &a;
  uchar* pb = &b;
  uchar* pr = &r;
  for(i=0; i<4; i++) {
    *pr = *pa ^ *pb;
    pr++;
    pa++;
    pb++;
  }
  return r;
}

/* calculate a checksum on a buffer */
uint32_t crc32_byte(uint8_t* p, uint32_t bytelength)
{
	uint32_t crc = 0xFFFFFFFFL;
	while (bytelength-- !=0) {
    crc = xor32(poly8_lookup[((uint8_t)crc^*(p++))], (crc>>8));
  }
	return (~crc);
}

/*
 * Send network packet
 */
uint ne2k_send(uchar* data, uint len)
{
  uint i;
	while(inb(base + CR) == 0x26) { /* Abort/Complete DMA + Transmit + Start */
  }

	outb(0, base + RSAR0);
	outb(0x40, base + RSAR1);
	outb(len & 0xFF, base + RBCR0);
	outb((len >> 8) & 0xFF, base + RBCR1);
	outb(0x12, base + CR); 	/* Write and start */

  for(i=0; i<len; i++) {
	  outb(data[i], base + DATA);
   }

	while((inb(base + ISR) & 0x40) == 0) { /* Wait for DMA completed */
  }

	outb(0x40, base + ISR); /* Clear bit */

	outb(0x40, base + TPSR );
	outb(len & 0xff, base + TBCR0);
	outb((len >> 8) & 0xff, base + TBCR1);
	outb(0x26, base + CR); /* Abort/Complete DMA + Transmit + Start */

	return 0;
}

/*
 * Send network packet (ethernet)
 */
uint eth_send(uchar* dst, uint type, uchar* data, uint len)
{
  uint  head_len = sizeof(struct ethhdr);
  struct ethhdr* eh = (struct ethhdr*)data;
  uint32_t eth_crc = 0;

  memcpy(eh->data, data, len);
  memcpy(eh->dst, dst, sizeof(eh->dst));
	memcpy(eh->src, local_mac, sizeof(eh->src));
	eh->type = bswap_16(type);

  eth_crc = crc32_byte(data, head_len+len);
  memcpy(&eh->data[len], &eth_crc, sizeof(eth_crc));
  return ne2k_send(data, len + head_len-1 + ETH_CRC_LEN);
}

/*
 * Request mac address given an IP
 */
uint arp_request(uchar* ip)
{
  uint head_len = sizeof(struct arphdr);
  uchar broadcast[ARP_HADDR_LEN];
  struct arphdr* ah = snd_buff;
  memset(broadcast, 0xFF, sizeof(broadcast));

  ah->hrd = bswap_16(ARP_HTYPE_ETHER);
  ah->pro = bswap_16(ARP_PTYPE_IP);
  ah->hln = ARP_HADDR_LEN;
  ah->pln = ARP_PADDR_LEN;
  ah->op = bswap_16(ARP_OP_REQUEST);
  memcpy(ah->sha, local_mac, sizeof(ah->sha));
  memcpy(ah->spa, local_ip, sizeof(ah->spa));
  memcpy(ah->dha, broadcast, sizeof(ah->dha));
  memcpy(ah->dpa, ip, sizeof(ah->dpa));
  return eth_send(broadcast, ETH_TYPE_ARP, snd_buff, head_len);
}

/*
 * Reply with local mac address
 */
uint arp_reply(uchar* hwdst, uchar* pst)
{
  uint head_len = sizeof(struct arphdr);
  struct arphdr* ah = snd_buff;

  ah->hrd = bswap_16(ARP_HTYPE_ETHER);
  ah->pro = bswap_16(ARP_PTYPE_IP);
  ah->hln = ARP_HADDR_LEN;
  ah->pln = ARP_PADDR_LEN;
  ah->op = bswap_16(ARP_OP_REPLY);
  memcpy(ah->sha, local_mac, sizeof(ah->sha));
  memcpy(ah->spa, local_ip, sizeof(ah->spa));
  memcpy(ah->dha, hwdst, sizeof(ah->dha));
  memcpy(ah->dpa, pst, sizeof(ah->dpa));
  return eth_send(hwdst, ETH_TYPE_ARP, snd_buff, head_len);
}

/*
 * Send IP packet
 */
uint ip_send(uchar* dst, uchar protocol, uchar* data, uint len)
{
  uint head_len = sizeof(struct IPv4hdr);
  struct IPv4hdr* ih = data;
  uint checksum = 0;
  uchar* hw_dst = 0;
  uint i = 0;

  memcpy(&(data[head_len]), data, len);
  ih->verIhl = (4<<4) | 5;
  ih->tos = 0;
  ih->len = bswap_16(len+head_len);
  ih->id = bswap_16(0);
  ih->offset = bswap_16(0);
  ih->ttl = 64;
  ih->protocol = protocol;
  ih->checksum = 0;
  memcpy(ih->src, local_ip, sizeof(ih->src));
  memcpy(ih->dst, dst, sizeof(ih->dst));

  checksum = net_checksum(data, len+head_len);
  ih->checksum = bswap_16(checksum);

  /* Try to find hw address in table */
  i = 0;
  for(i=0; i<4; i++) {
    if(dst[i]&local_net[i] != local_ip[i]) {
      break;
    }
  }

  /* Send to local gate if is outside network */
  if(i==4) {
    hw_dst = find_hw_in_table(dst);
  } else {
    hw_dst = find_hw_in_table(local_gate);
  }

  /* Unsuccessful */
  if(hw_dst == 0) {
    debugstr("net: IP: Can't find hw address for %d.%d.%d.%d. Aborted\n\r",
      dst[0], dst[1], dst[2], dst[3]);
    return 1;
  }

  return eth_send(hw_dst, ETH_TYPE_IP, data, head_len+len);
}

/*
 * Ensure hw address is in table
 * Ask for it, if it isn't
 */
uint provide_hw_address(uchar* dst)
{
  /* Try to find hw address */
  uint i = 0;
  uchar* hw_dst = find_hw_in_table(dst);

  while(hw_dst==0 && i<16) {
    debugstr("net: Requesting hw address for %d.%d.%d.%d...\n\r",
      dst[0], dst[1], dst[2], dst[3]);

    /* Request it and wait */
    arp_request(dst);
    wait(2000);

    /* Try again */
    hw_dst = find_hw_in_table(dst);
    i++;
  }

  /* Unsuccessful */
  if(hw_dst == 0) {
    return 1;
  }

  return 0;
}

/*
 * Send UDP packet
 */
uint udp_send(uchar* dst, uint src_port, uint dst_port, uchar protocol, uchar* data, uint len)
{
  uint head_len = sizeof(struct UDPhdr);
  struct UDPhdr* uh = snd_buff;
  uint checksum = 0;

  /* Provide hw addresss before process */
  if(provide_hw_address(dst) != 0 || provide_hw_address(local_gate)) {
    debugstr("net: can't find hw address for %d.%d.%d.%d. Aborted\n\r",
      dst[0], dst[1], dst[2], dst[3]);
    return 1;
  }

  /* Generate UDP packet and send */
  memcpy(&(snd_buff[head_len]), data, len);
  uh->srcPort = bswap_16(src_port);
  uh->dstPort = bswap_16(dst_port);
  uh->len = bswap_16(len+head_len);

  checksum = net_checksum(uh, len+head_len);
  uh->checksum = bswap_16(checksum);
  return ip_send(dst, IP_PROTOCOL_UDP, uh, head_len+len);
}

/*
 * Process received IP packet
 */
void ip_receive(uchar* buff, uint len)
{
  uint head_len = sizeof(struct IPv4hdr);
  struct IPv4hdr* ih = buff;

  /* Store only one packet */
  if(rcv_size > 0) {
    debugstr("net: packet received but discarded (buffer is full)\n\r");
    return;
  }

  /* Check UDP packet type */
  if(ih->protocol == IP_PROTOCOL_UDP) {
    uint i = 0;
    struct UDPhdr* uh = &buff[head_len];
    head_len = sizeof(struct UDPhdr);

    debugstr("net: UDP received: %u.%u.%u.%u:%u to port %u (%u bytes)\n\r",
      ih->src[0], ih->src[1], ih->src[2], ih->src[3],
      uh->srcPort, uh->dstPort, uh->len - head_len);

    /* Store it */
    if(uh->dstPort == NSUDP_PROTO_PORT) {
      rcv_port = uh->srcPort;
      rcv_size = uh->len - head_len;
      memcpy(rcv_addr, ih->src, sizeof(rcv_addr));
      memcpy(rcv_buff, &buff[head_len], rcv_size);
    }
  }
}

/*
 * Process received ARP packet
 */
void arp_receive(uchar* buff, uint len)
{
  struct arphdr* ah = buff;

  if(ah->hrd == bswap_16(ARP_HTYPE_ETHER) &&
    ah->pro == bswap_16(ARP_PTYPE_IP)) {
    /* If it's a reply */
    if(ah->op == bswap_16(ARP_OP_REPLY)) {
      /* If exists, update entry */
      uchar* hw = find_hw_in_table(ah->spa);
      if(hw) {
        memcpy(hw, ah->sha, sizeof(ah->sha));
        debugstr("net: ARP: updated: %d.%d.%d.%d : %2x:%2x:%2x:%2x:%2x:%2x\n\r",
          ah->spa[0], ah->spa[1], ah->spa[2], ah->spa[3],
          ah->sha[0], ah->sha[1], ah->sha[2], ah->sha[3], ah->sha[4], ah->sha[5]);
      } else { /* If does not exist, create new entry */
        uint i = 0;
        for(i=0; i<ARP_TABLE_LEN; i++) {
          if(arp_table[i].p[0] == 0) { /* Free entry */
            memcpy(arp_table[i].p, ah->spa, sizeof(arp_table[i].p));
            memcpy(arp_table[i].hw, ah->sha, sizeof(arp_table[i].hw));
            debugstr("net: ARP: added: %d.%d.%d.%d : %2x:%2x:%2x:%2x:%2x:%2x\n\r",
              ah->spa[0], ah->spa[1], ah->spa[2], ah->spa[3],
              ah->sha[0], ah->sha[1], ah->sha[2], ah->sha[3], ah->sha[4], ah->sha[5]);
            break;
          }
        }
      }
    } /* Answer if it's a local hw address request */
    else if(ah->op == bswap_16(ARP_OP_REQUEST) &&
      memcmp(ah->dpa, local_ip, sizeof(ah->dpa)) == 0) {
      arp_reply(ah->sha, ah->spa);
      debugstr("net: sent arp reply\n\r");
    }
  } else {
    debugstr("net: received an unknown type arp packet\n\r");
  }
}

/*
 * Receive network packet
 */
void ne2k_receive()
{
  uint i;
  struct ethhdr* eh = (struct ethhdr*)tmp_buff;

	struct {
		uchar rsr;
		uchar next;
		uint  len;
	} info;

	outb(0, base + RSAR0);
	outb(rx_next, base + RSAR1);
	outb(4, base + RBCR0);
	outb(0, base + RBCR1);
	outb(0x12, base + CR ); /* Read and start */

  for(i=0; i<4; i++) {
	  ((uchar*)&info)[i] = inb(base + DATA);
  }

	outb(4, base + RSAR0);
	outb(rx_next, base + RSAR1);
	outb(info.len & 0xff, base + RBCR0);
	outb((info.len >> 8) & 0xff, base + RBCR1);
  for(i=0; i<info.len; i++) {
	  tmp_buff[min(i, sizeof(tmp_buff)-1)] = inb(base + DATA);
  }

	while((inb(base + ISR) & 0x40) == 0) { /* Wait for DMA completed */
  }

  outb(0x40, base + ISR); /* Clear bit */

	rx_next = info.next;
	if(rx_next == 0x40) {
		outb(base + BNRY, 0x80);
  } else {
    outb(base + BNRY, rx_next - 1);
  }

/*	debugstr("eth dst: %2x:%2x:%2x:%2x:%2x:%2x\n\r", eh->dst[0], eh->dst[1],
    eh->dst[2], eh->dst[3], eh->dst[4], eh->dst[5]);
	debugstr("eth src: %2x:%2x:%2x:%2x:%2x:%2x\n\r", eh->src[0], eh->src[1],
		eh->src[2], eh->src[3], eh->src[4], eh->src[5]);*/

  /* Redirect packets to handlers */
  switch(bswap_16(eh->type)) {
	case ARP_PTYPE_IP:
		ip_receive(eh->data, info.len-sizeof(struct ethhdr)-1);
		break;
	case ARP_PTYPE_ARP: {
    arp_receive(eh->data, info.len-sizeof(struct ethhdr)-1);
		break;
  }
	default:
	 debugstr("net: received unknown type ethernet packet\n\r");
 };
}

/*
 * Interrupt handler
 */
void net_handler()
{
	uchar isr = inb(base + ISR);

	if(isr & NE2K_STAT_RX) {
    /* debugstr("net: packet received\n\r"); */
		outb(NE2K_STAT_RX, base + ISR); /* Clear bit */
		ne2k_receive();
		return;
	}
	if(isr & NE2K_STAT_TX) {
		/*debugstr("net: successfully sent packet\n\r");*/
		outb(NE2K_STAT_TX, base + ISR); /* Clear bit */
		return;
	}

	debugstr("net: unknown interrupt: %x\n\r", inb(base + ISR));
	outb(0xFF, base + ISR); /* Clear all bits */
}

/*
 * Initialize network
 */
void net_init()
{
  uint i;
  uchar* hw_gate;

  /* Reset translation table */
  memset(arp_table, 0, sizeof(arp_table));

  /* Reset received data */
  memset(rcv_addr, 0, sizeof(rcv_addr));
  rcv_port = 0;
  rcv_size = 0;

  /* Install handler */
	install_net_IRQ_handler();

  /* Reset, and wait */
	outb(inb(base + RESET), base + RESET);
	while((inb(base + ISR) & 0x80) == 0) {
	}

	debugstr("net: ne2k reset\n\r");

	outb(0x21, base + CR);  /* Stop DMA and MAC */
	outb(0x48, base + DCR); /* Access by words */
	outb(0x06, base + TCR); /* Transmit: normal operation, aut-append and check CRC */
	outb(0xD6, base + RCR); /* Receive: Accept and buffer */
	outb(0x00, base + IMR); /* Disable interrupts */
	outb(0xFF, base + ISR); /* ISR must be cleared */

	outb(0x40, base + TPSR ); /* Transmit page start */
	outb(rx_next - 1, base + PSTART); /* Receive page start */
	outb(0x80, base + PSTOP); /* Receive page stop */
	outb(rx_next - 1, base + BNRY); /* Boundary */
	outb(0x61, base + CR); /* Start, complete, config */
	outb(rx_next, base + CURR); /* Change page */
	outb(0x21, base + CR);

	outb(0x00, base + RSAR0); /* Remote start address */
	outb(0x00, base + RSAR1);
	outb(24, base + RBCR0); /* 24 bytes count */
	outb(0x00, base + RBCR1);
	outb(0x0A, base + CR);

  /* Print MAC */
  debugstr("net: MAC: ");
	for(i=0; i<6; i++) {
		local_mac[i] = inb(base + DATA);
    inb(base + DATA);
		debugstr("%2x ", local_mac[i]);
	}
	debugstr("\n\r");

  /* Listen to this MAC */
	outb(0x61, base + CR);
	outb(local_mac[0], base + PAR0);
	outb(local_mac[1], base + PAR1);
	outb(local_mac[2], base + PAR2);
	outb(local_mac[3], base + PAR3);
	outb(local_mac[4], base + PAR4);
	outb(local_mac[5], base + PAR5);

  /* Clear multicast */
  for(i=MAR0; i<=MAR7; i++) {
    outb(0, base + i);
  }

  outb(0x22, base + CR); /* Start and no DMA */
  outb(0x03, base + IMR); /* Set interrupt mask to read/write */

  /* Add local computer to table */
  memset(arp_table[0].hw, 0xFF, sizeof(arp_table[0].hw));
  memset(arp_table[0].p, 0xFF, sizeof(arp_table[0].p));
}

/*
 * Test network
 */
void net_test()
{
  /* Send test packet */
  debugstr("net: sending test packet...\n\r");
  net_send(local_gate, test_data, sizeof(test_data));
  net_send(local_ip, test_data, sizeof(test_data));
}

/*
 * Send buffer to dstIP
 */
uint net_send(uchar* dstIP, uchar* buff, uint len)
{
  return udp_send(dstIP, NSUDP_PROTO_PORT, NSUDP_PROTO_PORT,
    IP_PROTOCOL_UDP, buff, len);
}

/*
 * Receive data
 */
uint net_recv(uchar* srcIP, uchar* buff, uint buff_size)
{
  if(rcv_size > 0) {
    uint ret = min(rcv_size, buff_size);
    memcpy(srcIP, rcv_addr, sizeof(rcv_addr));
    memcpy(buff, rcv_buff, ret);
    rcv_size = 0;
    return ret;
  }
  return 0;
}
