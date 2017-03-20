/*
 * PCI
 */

#ifndef _PCI_H
#define _PCI_H

#define PCI_CONFIG_ADDR_PORT 0xCF8
#define PCI_CONFIG_DATA_PORT 0xCFC

/* Offset in PIC config space */
#define PCI_VENDOR_ID        0x00
#define PCI_DEVICE_ID        0x02
#define PCI_COMMAND          0x04
#define PCI_STATUS           0x06
#define PCI_REVISION_ID      0x08
#define PCI_PROG_IF          0x09
#define PCI_SUBCLASS         0x0A
#define PCI_CLASSCODE        0x0B
#define PCI_CACHE_LINE_SIZE  0x0C
#define PCI_LATENCY_TIMER    0x0D
#define PCI_HEADER_TYPE      0x0E
#define PCI_BIST             0x0F
#define PCI_BAR0             0x10
#define PCI_BAR1             0x14
#define PCI_BAR2             0x18
#define PCI_BAR3             0x1C
#define PCI_BAR4             0x20
#define PCI_BAR5             0x24

#define PCI_INTERRUPT_LINE	 0x3C

struct PCI_DEVICE {
	uint32_t  dev; /* pci config addr base */
	uint16_t  vendor_id;
	uint16_t  device_id;
	uint16_t  command;
	uint16_t  status;
	uint8_t   revison_id;
	uint8_t   prog_if;
	uint8_t   subclass;
	uint8_t   class_code;
	uint8_t   cache_line_size;
	uint8_t   latency_timer;
	uint8_t   header_type;
	uint8_t   bist;
	uint32_t  bar0;
	uint32_t  bar1;
	uint32_t  bar2;
	uint32_t  bar3;
	uint32_t  bar4;
	uint32_t  bar5;
	uint32_t  crdbus_cis_pointer;
	uint16_t  subsystem_vendor_id;
	uint16_t  subsustem_id;
	uint32_t  eROM_base;
	uint8_t   capabilities_pointer;
	uint8_t   reserved1[3];
	uint32_t  reserved2;
	uint8_t   interrput_line;
	uint8_t   interrput_pin;
	uint8_t   min_grant;
	uint8_t   max_latency;
};

/*
 * Initialize PCI
 */
void pci_init();

/*
 * Find device
 */
struct PCI_DEVICE* pci_find_device(uint16_t vendor, uint16_t device);


#endif   /* _PCI_H */
