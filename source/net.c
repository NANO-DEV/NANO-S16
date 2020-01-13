#include "types.h"
#include "hw86.h"
#include "pci.h"
#include "ulib/ulib.h"
#include "net.h"

/* Network controller
 * Assumes NE2000 compatible nic
 * Example: Realtek RTL8019AS
 * http://www.ethernut.de/pdf/8019asds.pdf */

/*
 * The Ne2000 network card uses two ring buffers for packet handling.
 * These are circular buffers made of 256-byte pages that the chip's DMA logic
 * will use to store received packets or to get received packets.
 * Note that a packet will always start on a page boundary,
 * thus there may be unused bytes at the end of a page.
 *
 * Two registers NE2K_PSTART and NE2K_PSTOP define a set of 256-byte pages in the buffer
 * memory that will be used for the ring buffer. As soon as the DMA attempts to
 * read/write to NE2K_PSTOP, it will be sent back to NE2K_PSTART
 *
 * NE2K_PSTART                                                                       NE2K_PSTOP
 * ####+-8------+-9------+-a------+-b------+-c------+-d------+-e------+-f------+####
 * ####| Packet 3 (cont) |########|########|Packet1#|   Packet  2#####|Packet 3|####
 * ####+--------+--------+--------+--------+--------+--------+--------+--------+####
 * (An 8-page ring buffer with 3 packets and 2 free slots)
 * While receiving, the NIC has 2 additional registers that point to the first
 * packet that's still to be read and to the start of the currently written
 * packet (named boundary pointer and current page respectively).
 *
 * Programming registers of the NE2000 are collected in pages.
 * Page 0 contains most of the control and status registers while
 * page 1 contains physical (NE2K_PAR0..NE2K_PAR5) and multicast addresses (NE2K_MAR0..NE2K_MAR7)
 * to be checked by the card
 */

/* Is network enabled */
uint network_enabled = 1;
uint net_irq = 9; /* network irq */

/* Install nic IRQ handler */
extern void install_net_IRQ_handler();

/* Byte swap operation */
#define BSWAP_16(value) \
((((value) & 0xff) << 8) | ((value) >> 8))

#define NUM_COMPATIBLE_DEVICES 1
struct device_info {
  uint16_t  vendor_id;
  uint16_t  device_id;
} ne2k_compatible[NUM_COMPATIBLE_DEVICES] = {
  0x10EC, 0x8029, /* Realtek */
};

/* Registers */
#define NE2K_CR       0x00 /* Command register */
/* 7-6:PS1-PS0 5-3:RD2-0 2:TXP 1:STA 0:STP */

/* Page 0 registers, read */
#define NE2K_CLDA0    0x01 /* Current Local DMA Address */
#define NE2K_CLDA1    0x02
#define NE2K_BNRY     0x03 /* Boundary */
#define NE2K_TSR      0x04 /* Transmit status */
#define NE2K_NCR      0x05 /* Collision counter */
#define NE2K_FIFO     0x06 /* Allows to examine the contents of the FIFO after loopback */
#define NE2K_ISR      0x07 /* Interrupt status */
#define NE2K_CRDA0    0x08 /* Current Remote DMA Address */
#define NE2K_CRDA1    0x09
#define NE2K_RSR      0x0C /* Receive status */
#define NE2K_CNTR0    0x0D /* Error counters */
#define NE2K_CNTR1    0x0E
#define NE2K_CNTR2    0x0F

/* Page 0 registers, write */
#define NE2K_PSTART   0x01 /* Page start (read page 2) */
#define NE2K_PSTOP    0x02 /* Page stop (read page 2) */
#define NE2K_TPSR     0x04 /* Transmit page start (read page 2) */
#define NE2K_TBCR0    0x05 /* Transmit byte count */
#define NE2K_TBCR1    0x06
#define NE2K_RSAR0    0x08 /* Remote start address */
#define NE2K_RSAR1    0x09
#define NE2K_RBCR0    0x0A /* Remote byte count */
#define NE2K_RBCR1    0x0B
#define NE2K_RCR      0x0C /* Receive config (read page 2) */
#define NE2K_TCR      0x0D /* Transmit config (read page 2) */
#define NE2K_DCR      0x0E /* Data config (read page 2) */
#define NE2K_IMR      0x0F /* Interrupt mask (read page 2) */

/* Page 1 registers, read/write */
#define NE2K_PAR0    0x01 /* Physical address */
#define NE2K_PAR1    0x02
#define NE2K_PAR2    0x03
#define NE2K_PAR3    0x04
#define NE2K_PAR4    0x05
#define NE2K_PAR5    0x06
#define NE2K_CURR    0x07 /* Current page */
#define NE2K_MAR0    0x08 /* Multicast address */
#define NE2K_MAR1    0x09
#define NE2K_MAR2    0x0A
#define NE2K_MAR3    0x0B
#define NE2K_MAR4    0x0C
#define NE2K_MAR5    0x0D
#define NE2K_MAR6    0x0E
#define NE2K_MAR7    0x0F

#define NE2K_DATA    0x10 /* Data i/o */
#define NE2K_RESET   0x1F /* Reset register */

/* NE2K_ISR/NE2K_IMR flags */
#define NE2K_STAT_RX 0x0001 /* Packet received */
#define NE2K_STAT_TX 0x0002 /* Packet sent */

/* Store here current reception page */
static uint rx_next = 0x47;

/* Hardware info */
#define MAC_LEN 6 /* Bytes size of a MAC address */
static uint base = 0x300; /* Base device port */
static uint8_t local_mac[MAC_LEN]; /* Get from network card */

/* IP protocol network  params */
uint8_t local_ip[IP_LEN] = {192,168,0,40}; /* Default value, global */
uint8_t local_gate[IP_LEN] = {192,168,0,1}; /* Default value, global */
uint8_t local_net[IP_LEN] = {255,255,255,0}; /* Default value, global */

/* Ethernet related */
typedef struct {
  uint8_t  dst[MAC_LEN];
  uint8_t  src[MAC_LEN];
  uint16_t type;
  uint8_t  data[0]; /* size 46-1500 */
} eth_hdr_t;

/* Values of ethhdr->eh_type */
#define ETH_TYPE_ARP  0x0806
#define ETH_TYPE_IP   0x0800

#define ETH_HDR_LEN   14

#define ETH_MTU       1500
#define ETH_VLAN_LEN  4
#define ETH_CRC_LEN   4

#define ETH_PKT_MAX_LEN (ETH_HDR_LEN+ETH_VLAN_LEN+ETH_MTU)

/* ARP related */
typedef struct {
  uint16_t hrd;   /* format of hardware address */
  uint16_t pro;   /* format of protocol address */
  uint8_t  hln;   /* length of hardware address */
  uint8_t  pln;   /* length of protocol address */
  uint16_t op;    /* arp/rarp operation */
  uint8_t  sha[MAC_LEN];
  uint8_t  spa[IP_LEN];
  uint8_t  dha[MAC_LEN];
  uint8_t  dpa[IP_LEN];
} arp_hdr_t;

/* Values of arphdr->ah_hrd */
#define ARP_HTYPE_ETHER 1 /* Ethernet hardware type */

/* Values of arphdr->ah_pro */
#define ARP_PTYPE_IP    0x0800 /* IP protocol type */
#define ARP_PTYPE_ARP   0x0806 /* ARP protocol type */

/* Values of arphdr->ah_op */
#define ARP_OP_REQUEST  1 /* Request op code */
#define ARP_OP_REPLY    2 /* Reply op code */


/* IP Protocol */
#define IP_PROTOCOL_ICMP 1
#define IP_PROTOCOL_TCP  6
#define IP_PROTOCOL_UDP  17

/* IPv4 Header */
typedef struct {
  uint8_t  verIhl;
  uint8_t  tos;
  uint16_t len;
  uint16_t id;
  uint16_t offset;
  uint8_t  ttl;
  uint8_t  protocol;
  uint16_t checksum;
  uint8_t  src[IP_LEN];
  uint8_t  dst[IP_LEN];
} ip_hdr_t;

/* UDP header */
typedef struct {
  uint16_t srcPort;
  uint16_t dstPort;
  uint16_t len;
  uint16_t checksum;
} udp_hdr_t;

/* ARP table to hold IP-MAC entries */
#define ARP_TABLE_LEN 8
struct arp_table_info {
  uint8_t ip[IP_LEN];
  uint8_t mac[MAC_LEN];
} arp_table[ARP_TABLE_LEN];

/* Given an IP address, provide effective IP address to send packet */
uint8_t* get_effective_ip(uint8_t* ip)
{
  uint i = 0;

  /* If IP is outside the local network return gate address */
  for(i=0; i<IP_LEN; i++) {
    if((ip[i]&local_net[i]) != (local_ip[i]&local_net[i])) {
      break;
    }
  }
  if(i!=IP_LEN) {
    return local_gate;
  }
  /* Otherwise, return the same IP */
  return ip;
}

/* Given an IP address, provide MAC address
 * if found in the translation table */
uint8_t* find_mac_in_table(uint8_t* ip)
{
  uint i = 0;

  /* Local IP -> local MAC */
  if(memcmp(ip, local_ip, sizeof(local_ip)) == 0) {
    return local_mac;
  }

  /* Otherwise search in the table */
  for(i=0; i<ARP_TABLE_LEN; i++) {
    if(memcmp(arp_table[i].ip, ip,
      sizeof(arp_table[i].ip)) == 0) {
      return arp_table[i].mac;
    }
  }
  return 0;
}

/* Buffers to send/receive packets */
uint8_t tmp_buff[256];
uint8_t rcv_buff[256];
uint8_t rcv_addr[IP_LEN];
uint    rcv_port = 0;
uint    rcv_size = 0;
uint8_t snd_buff[256];

/* For now, lets save only what is received in this port */
#define NSUDP_PROTO_PORT 8086


/* Keep checksum 16-bits */
uint16_t net_checksum_final(uint32_t sum)
{
  uint16_t temp;
  sum = (sum&0xFFFFL) + (sum>>16);

  temp = ~sum;
  return ((temp&0x00FF)<<8) | ((temp&0xFF00)>>8);
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
  uint i = 0;
  uint32_t r = 0;
  uint8_t* pa = &a;
  uint8_t* pb = &b;
  uint8_t* pr = &r;
  for(i=0; i<4; i++) {
    *pr = *pa ^ *pb;
    pr++;
    pa++;
    pb++;
  }
  return r;
}

/* Calculate a checksum on a buffer */
uint32_t crc32_byte(uint8_t* p, uint32_t bytelength)
{
  uint32_t crc = 0xFFFFFFFFL;
  while(bytelength-- !=0) {
    crc = xor32(poly8_lookup[((uint8_t)(crc&0xFF)^(*(p++)))], (crc>>8));
  }
  return (~crc);
}

/* Select a registers page in the ne2k */
void ne2k_page_select(uint page)
{
  uint pg = (page&0x01)<<6;
  uint cm = 0x3F & inb(base + NE2K_CR);
  outb((uchar)(pg|cm), base + NE2K_CR);
}

/*
 * Send network packet (hardware, ne2k)
 */
uint ne2k_send(uint8_t* data, uint len)
{
  uint i = 0;
  while(inb(base + NE2K_CR) == 0x26) { /* Abort/Complete DMA + Transmit + Start */
  }

  /* Prepare buffer and size */
  ne2k_page_select(0);
  outb(0, base + NE2K_RSAR0);
  outb(0x40, base + NE2K_RSAR1);
  outb(len & 0xFF, base + NE2K_RBCR0);
  outb((len >> 8) & 0xFF, base + NE2K_RBCR1);

  outb(0x12, base + NE2K_CR);  /* Start write */

  /* Load buffer */
  for(i=0; i<len; i++) {
    outb(data[i], base + NE2K_DATA);
   }

  /* Wait until operation completed */
  while((inb(base + NE2K_ISR) & 0x40) == 0) {
  }

  outb(0x40, base + NE2K_ISR); /* Clear completed bit */

  outb(0x40, base + NE2K_TPSR);
  outb((len & 0xFF), base + NE2K_TBCR0);
  outb(((len >> 8) & 0xFF), base + NE2K_TBCR1);
  outb(0x26, base + NE2K_CR); /* Abort/Complete DMA + Transmit + Start */

  return 0;
}

/*
 * Send network packet (ethernet)
 */
uint eth_send(uint8_t* dst_mac, uint type, uint8_t* data, uint len)
{
  uint head_len = sizeof(eth_hdr_t);
  eth_hdr_t* eh = (eth_hdr_t*)data;
  uint32_t eth_crc = 0;

  memcpy(eh->data, data, len);
  memcpy(eh->dst, dst_mac, sizeof(eh->dst));
  memcpy(eh->src, local_mac, sizeof(eh->src));
  eh->type = BSWAP_16(type);

  eth_crc = crc32_byte(data, head_len+len);
  memcpy(&eh->data[len], &eth_crc, sizeof(eth_crc));
  return ne2k_send(data, len + head_len + ETH_CRC_LEN);
}

/*
 * Request mac address given an IP
 */
uint arp_request(uint8_t* ip)
{
  uint head_len = sizeof(arp_hdr_t);
  uint8_t broadcast_mac[MAC_LEN];
  arp_hdr_t* ah = snd_buff;
  memset(broadcast_mac, 0xFF, sizeof(broadcast_mac));

  ah->hrd = BSWAP_16(ARP_HTYPE_ETHER);
  ah->pro = BSWAP_16(ARP_PTYPE_IP);
  ah->hln = MAC_LEN;
  ah->pln = IP_LEN;
  ah->op = BSWAP_16(ARP_OP_REQUEST);
  memcpy(ah->sha, local_mac, sizeof(ah->sha));
  memcpy(ah->spa, local_ip, sizeof(ah->spa));
  memcpy(ah->dha, broadcast_mac, sizeof(ah->dha));
  memcpy(ah->dpa, ip, sizeof(ah->dpa));
  return eth_send(broadcast_mac, ETH_TYPE_ARP, snd_buff, head_len);
}

/*
 * Reply an ARP request with local mac address
 */
uint arp_reply(uint8_t* mac, uint8_t* ip)
{
  uint head_len = sizeof(arp_hdr_t);
  arp_hdr_t* ah = snd_buff;

  ah->hrd = BSWAP_16(ARP_HTYPE_ETHER);
  ah->pro = BSWAP_16(ARP_PTYPE_IP);
  ah->hln = MAC_LEN;
  ah->pln = IP_LEN;
  ah->op = BSWAP_16(ARP_OP_REPLY);
  memcpy(ah->sha, local_mac, sizeof(ah->sha));
  memcpy(ah->spa, local_ip, sizeof(ah->spa));
  memcpy(ah->dha, mac, sizeof(ah->dha));
  memcpy(ah->dpa, ip, sizeof(ah->dpa));
  return eth_send(mac, ETH_TYPE_ARP, snd_buff, head_len);
}

/*
 * Send IP packet
 */
uint ip_send(uint8_t* dst_ip, uint8_t protocol, uint8_t* data, uint len)
{
  uint head_len = sizeof(ip_hdr_t);
  ip_hdr_t* ih = data;
  uint checksum = 0;
  uint8_t* dst_mac = 0;
  uint i = 0;
  static uint id = 0;

  memcpy(&(data[head_len]), data, len);
  ih->verIhl = (4<<4) | 5;
  ih->tos = 0;
  ih->len = BSWAP_16(len+head_len);
  ih->id = BSWAP_16(++id);
  ih->offset = BSWAP_16(0);
  ih->ttl = 128;
  ih->protocol = protocol;
  ih->checksum = 0;
  memcpy(ih->src, local_ip, sizeof(ih->src));
  memcpy(ih->dst, dst_ip, sizeof(ih->dst));

  checksum = net_checksum(data, head_len);
  ih->checksum = BSWAP_16(checksum);

  /* Try to find hw address in table */
  dst_mac = find_mac_in_table(get_effective_ip(dst_ip));

  /* Unsuccessful */
  if(dst_mac == 0) {
    debugstr("net: IP: Can't find hw address for %d.%d.%d.%d. Aborted\n\r",
      dst_ip[0], dst_ip[1], dst_ip[2], dst_ip[3]);
    return 1;
  }

  return eth_send(dst_mac, ETH_TYPE_IP, data, head_len+len);
}

/*
 * Ensure mac address is in table
 * Ask for it, if it isn't
 */
uint provide_mac_address(uint8_t* ip)
{
  uint i = 0;
  uint8_t* mac = 0;

  /* Get effective address */
  ip = get_effective_ip(ip);

  /* Find in table */
  mac = find_mac_in_table(ip);

  /* If not found, request */
  while(mac==0 && i<16) {
    debugstr("net: Requesting mac for %d.%d.%d.%d...\n\r",
      ip[0], ip[1], ip[2], ip[3]);

    /* Request it and wait */
    arp_request(ip);
    wait(1000);

    /* Try again */
    mac = find_mac_in_table(ip);
    i++;
  }

  /* Unsuccessful */
  if(mac == 0) {
    return 1;
  }

  return 0;
}

/*
 * Send UDP packet
 */
uint udp_send(uint8_t* dst_ip, uint src_port, uint dst_port,
  uint8_t protocol, uint8_t* data, uint len)
{
  typedef struct {
    uint8_t  sender[4];
    uint8_t  recver[4];
    uint8_t  zero;
    uint8_t  protocol;
    uint16_t len;
  } udpip_hdr_t;

  uint head_len = sizeof(udp_hdr_t);
  uint iphead_len = sizeof(udpip_hdr_t);
  udpip_hdr_t* ih = snd_buff;
  udp_hdr_t* uh = &snd_buff[iphead_len];
  uint checksum = 0;

  /* Provide hw addresss before process */
  if(provide_mac_address(dst_ip) != 0 || provide_mac_address(local_gate) != 0) {
    debugstr("net: can't find hw address for %d.%d.%d.%d. Aborted\n\r",
      dst_ip[0], dst_ip[1], dst_ip[2], dst_ip[3]);
    return 1;
  }

  /* Clamp len */
  len = min(sizeof(snd_buff) - iphead_len-head_len -
    sizeof(ip_hdr_t) - sizeof(eth_hdr_t),
    len);

  /* Generate UDP packet and send */
  memset(snd_buff, 0, sizeof(snd_buff));
  memcpy(&(snd_buff[iphead_len+head_len]), data, len);

  uh->srcPort = BSWAP_16(src_port);
  uh->dstPort = BSWAP_16(dst_port);
  uh->len = BSWAP_16(len+head_len);

  memcpy(ih->sender, local_ip, sizeof(ih->sender));
  memcpy(ih->recver, dst_ip, sizeof(ih->recver));
  ih->zero = 0;
  ih->protocol = IP_PROTOCOL_UDP;
  ih->len = uh->len;
  checksum = net_checksum(snd_buff, iphead_len+len+head_len);
  uh->checksum = BSWAP_16(checksum);
  return ip_send(dst_ip, IP_PROTOCOL_UDP, uh, head_len+len);
}

/*
 * Process received IP packet
 */
void ip_recv_process(uint8_t* buff, uint len)
{
  uint head_len = sizeof(ip_hdr_t);
  ip_hdr_t* ih = buff;

  /* Store only one packet */
  if(rcv_size > 0) {
    debugstr("net: packet received but discarded (buffer is full)\n\r");
    return;
  }

  /* Check UDP packet type */
  if(ih->protocol == IP_PROTOCOL_UDP) {
    uint i = 0;
    udp_hdr_t* uh;
    buff += head_len; /* Advance buffer */
    uh = buff;
    head_len = sizeof(udp_hdr_t);

    debugstr("net: UDP received: %u.%u.%u.%u:%u to port %u (%u bytes)\n\r",
      ih->src[0], ih->src[1], ih->src[2], ih->src[3],
      BSWAP_16(uh->srcPort), BSWAP_16(uh->dstPort), BSWAP_16(uh->len) - head_len);

    /* Store it */
    if(BSWAP_16(uh->dstPort) == NSUDP_PROTO_PORT) {
      rcv_port = BSWAP_16(uh->srcPort);
      rcv_size = min(BSWAP_16(uh->len)-head_len, sizeof(rcv_buff));
      memcpy(rcv_addr, ih->src, sizeof(rcv_addr));
      memcpy(rcv_buff, &buff[head_len], rcv_size);
      debugstr("net: UDP packet was stored\n\r");
    }
  }
}

/*
 * Process received ARP packet
 */
void arp_recv_process(uint8_t* buff, uint len)
{
  arp_hdr_t* ah = buff;

  if(ah->hrd == BSWAP_16(ARP_HTYPE_ETHER) &&
    ah->pro == BSWAP_16(ARP_PTYPE_IP)) {
    /* If it's a reply */
    if(ah->op == BSWAP_16(ARP_OP_REPLY) &&
      memcmp(ah->dpa, local_ip, sizeof(ah->dpa)) == 0) {
      /* If exists in table, update entry */
      uint8_t* mac = find_mac_in_table(ah->spa);
      if(mac) {
        memcpy(mac, ah->sha, sizeof(ah->sha));
        debugstr("net: ARP: updated: %d.%d.%d.%d : %2x:%2x:%2x:%2x:%2x:%2x\n\r",
          ah->spa[0], ah->spa[1], ah->spa[2], ah->spa[3],
          ah->sha[0], ah->sha[1], ah->sha[2], ah->sha[3], ah->sha[4], ah->sha[5]);
      } else { /* If does not exist, create new entry */
        uint i = 0;
        for(i=0; i<ARP_TABLE_LEN; i++) {
          if(arp_table[i].ip[0] == 0 || i==ARP_TABLE_LEN-1) { /* Free entry */
            memcpy(arp_table[i].ip, ah->spa, sizeof(arp_table[i].ip));
            memcpy(arp_table[i].mac, ah->sha, sizeof(arp_table[i].mac));
            debugstr("net: ARP: added: %d.%d.%d.%d : %2x:%2x:%2x:%2x:%2x:%2x\n\r",
              ah->spa[0], ah->spa[1], ah->spa[2], ah->spa[3],
              ah->sha[0], ah->sha[1], ah->sha[2], ah->sha[3], ah->sha[4], ah->sha[5]);
            break;
          }
        }
      }
    /* Reply in case it's a local MAC address request */
    } else if(ah->op == BSWAP_16(ARP_OP_REQUEST)) {
      if(memcmp(ah->dpa, local_ip, sizeof(ah->dpa)) == 0) {
        arp_reply(ah->sha, ah->spa);
        debugstr("net: sent arp reply\n\r");
      }
    }
  } else {
  }
}

/*
 * Receive network packet
 */
void ne2k_receive()
{
  uint i = 0;
  eth_hdr_t* eh = (eth_hdr_t*)tmp_buff;

  struct {
    uint8_t rsr;
    uint8_t next;
    uint    len;
  } info;

  /* Maybe more than one packet is in buffer */
  /* Retrieve all of them */
  uint8_t bndry = 0;
  uint8_t current = 0;

  ne2k_page_select(1);
  current = inb(base + NE2K_CURR);
  ne2k_page_select(0);
  bndry = inb(base + NE2K_BNRY);

  while(bndry != current) {
    /* Get reception info */
    ne2k_page_select(0);
    outb(0, base + NE2K_RSAR0);
    outb(rx_next, base + NE2K_RSAR1);
    outb(4, base + NE2K_RBCR0);
    outb(0, base + NE2K_RBCR1);
    outb(0x12, base + NE2K_CR); /* Read and start */

    for(i=0; i<4; i++) {
      ((uint8_t*)&info)[i] = inb(base + NE2K_DATA);
    }

    /* Get the data */
    outb(4, base + NE2K_RSAR0);
    outb(rx_next, base + NE2K_RSAR1);

    outb((info.len & 0xFF), base + NE2K_RBCR0);
    outb(((info.len >> 8) & 0xFF), base + NE2K_RBCR1);

    outb(0x12, base + NE2K_CR); /* Read and start */

    for(i=0; i<info.len; i++) {
      tmp_buff[min(i, sizeof(tmp_buff)-1)] = inb(base + NE2K_DATA);
    }

    /* Wait for operation completed */
    while((inb(base + NE2K_ISR) & 0x40) == 0) {
    }
    /* Clear completed bit */
    outb(0x40, base + NE2K_ISR);

    /* Update reception pages */
    if(info.next) {
      rx_next = info.next;
      if(rx_next == 0x40) {
        outb(0x80, base + NE2K_BNRY);
      } else {
        outb(rx_next==0x46?0x7F:rx_next-1, base + NE2K_BNRY);
      }
    }

    /* Update current and bndry values */
    ne2k_page_select(1);
    current = inb(base + NE2K_CURR);
    ne2k_page_select(0);
    bndry = inb(base + NE2K_BNRY);

    /* Wait and clear */
    while((inb(base + NE2K_ISR) & 0x40) == 0) {
    }
    outb(0x40, base + NE2K_ISR);

    /* Process packet if broadcast or unicast to local_mac */
    if(!memcmp(eh->dst, local_mac, sizeof(eh->dst)) ||
      !memcmp(eh->dst, arp_table[0].mac, sizeof(eh->dst)))
    {
      /* Clamp length to reception buffer size */
      info.len = min(sizeof(tmp_buff), info.len);

      /* Redirect packets to type handlers */
      switch(BSWAP_16(eh->type)) {
      case ARP_PTYPE_IP:
        ip_recv_process(eh->data, info.len-sizeof(eth_hdr_t));
        break;
      case ARP_PTYPE_ARP:
        arp_recv_process(eh->data, info.len-sizeof(eth_hdr_t));
        break;
      };
    }

    /* Break if no more packets */
    if(info.next == current || !info.next) {
      break;
    }
  }
}

/*
 * ne2k interrupt handler
 */
void net_handler()
{
  uint8_t isr = 0;

  /* Network must be enabled */
  if(!network_enabled) {
    return;
  }

  /* Iterate because more interrupts
   * can be received while handling previous */
  while((isr = inb(base + NE2K_ISR)) != 0) {
    if(isr & NE2K_STAT_RX) {
      ne2k_receive();
    }
    if(isr & NE2K_STAT_TX) {
    }

    /* Clear interrupt bits */
    outb(isr, base + NE2K_ISR);
  }
  return;
}

/*
 * Initialize network
 */
void net_init()
{
  uint i = 0;

  /* Reset translation table */
  memset(arp_table, 0, sizeof(arp_table));

  /* Reset received data */
  memset(rcv_addr, 0, sizeof(rcv_addr));
  rcv_port = 0;
  rcv_size = 0;

  /* Detect card */
  if(network_enabled == 1) {
    pci_device_t* pdev;
    network_enabled = 0;

    /* Find a compatible device */
    for(i=0; i<NUM_COMPATIBLE_DEVICES; i++) {
      pdev = pci_find_device(ne2k_compatible[i].vendor_id,
        ne2k_compatible[i].device_id);

      if(pdev) {
        break;
      }
    }

    /* If found, check */
    if(pdev) {
      base = pdev->bar0 & ~3;
      net_irq = pdev->interrput_line;
      outb(0x80, base + NE2K_IMR); /* Disable interrupts except reset */
      outb(0xFF, base + NE2K_ISR); /* Clear interrupts */
      outb(inb(base + NE2K_RESET), base + NE2K_RESET); /* Reset */
      wait(250); /* Wait */
      if((inb(base + NE2K_ISR) == 0x80)) { /* Detect reset interrupt */
        debugstr("net: ne2000 compatible nic found. base=%x irq=%x\n\r", base, net_irq);
        network_enabled = 1;
      }
    }
  }

  /* Abort if network is not enabled or device not found */
  if(network_enabled == 0) {
    debugstr("net: compatible nic not found\n\r");
    return;
  }

  /* Install handler */
  install_net_IRQ_handler();

  /* Reset, and wait */
  outb(inb(base + NE2K_RESET), base + NE2K_RESET);
  while((inb(base + NE2K_ISR) & 0x80) == 0) {
  }

  debugstr("net: nic reset\n\r");

  ne2k_page_select(0);
  outb(0x21, base + NE2K_CR);  /* Stop DMA and MAC */
  outb(0x48, base + NE2K_DCR); /* Access by bytes */
  outb(0xE0, base + NE2K_TCR); /* Transmit: normal operation, aut-append and check CRC */
  outb(0xDE, base + NE2K_RCR); /* Receive: Accept and buffer */
  outb(0x00, base + NE2K_IMR); /* Disable interrupts */
  outb(0xFF, base + NE2K_ISR); /* NE2K_ISR must be cleared */

  outb(0x40, base + NE2K_TPSR );       /* Transmit page start */
  outb(rx_next-1, base + NE2K_PSTART); /* Receive page start */
  outb(0x80, base + NE2K_PSTOP);       /* Receive page stop */
  outb(rx_next-1, base + NE2K_BNRY);   /* Boundary */
  ne2k_page_select(1);
  outb(rx_next, base + NE2K_CURR);     /* Change current recv page */

  ne2k_page_select(0);
  outb(0x00, base + NE2K_RSAR0);       /* Remote start address */
  outb(0x00, base + NE2K_RSAR1);
  outb(24, base + NE2K_RBCR0);         /* 24 bytes count */
  outb(0x00, base + NE2K_RBCR1);
  outb(0x0A, base + NE2K_CR);

  /* Print MAC */
  debugstr("net: MAC: ");
  for(i=0; i<6; i++) {
    local_mac[i] = inb(base + NE2K_DATA);
    inb(base + NE2K_DATA); /* Word sized, read again to advance */
    debugstr("%2x ", local_mac[i]);
  }
  debugstr("\n\r");

  /* Listen to this MAC */
  ne2k_page_select(1);
  outb(local_mac[0], base + NE2K_PAR0);
  outb(local_mac[1], base + NE2K_PAR1);
  outb(local_mac[2], base + NE2K_PAR2);
  outb(local_mac[3], base + NE2K_PAR3);
  outb(local_mac[4], base + NE2K_PAR4);
  outb(local_mac[5], base + NE2K_PAR5);

  /* Clear multicast */
  for(i=NE2K_MAR0; i<=NE2K_MAR7; i++) {
    outb(0, base + i);
  }

  ne2k_page_select(0);
  outb(0x22, base + NE2K_CR); /* Start and no DMA */
  outb(0x03, base + NE2K_IMR); /* Set interrupt mask to read/write */

  /* Add broadcast address to translation table */
  memset(arp_table[0].mac, 0xFF, sizeof(arp_table[0].mac));
  memset(arp_table[0].ip, 0xFF, sizeof(arp_table[0].ip));
}

/*
 * Send buffer to dstIP
 */
uint net_send(uint8_t* dst_ip, uint8_t* buff, uint len)
{
  if(network_enabled) {
    return udp_send(dst_ip, NSUDP_PROTO_PORT, NSUDP_PROTO_PORT,
      IP_PROTOCOL_UDP, buff, len);
      return 0;
  }
  return 1;
}

/*
 * Receive data
 */
uint net_recv(uint8_t* src_ip, uint8_t* buff, uint buff_size)
{
  if(network_enabled) {
    /* Handle pending nic requests */
    net_handler();

    /* Now check if there is something in the system buffer */
    if(rcv_size > 0) {
      uint ret = min(rcv_size, buff_size);
      memcpy(src_ip, rcv_addr, sizeof(rcv_addr));
      memcpy(buff, rcv_buff, ret);
      rcv_size = 0;
      return ret;
    }
  }
  return 0;
}
