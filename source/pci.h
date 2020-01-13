/*
 * PCI
 */

#ifndef _PCI_H
#define _PCI_H

/* Header type 0x00, general device */
typedef struct {
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
  uint32_t  cardbus_cis_pointer;
  uint16_t  subsystem_vendor_id;
  uint16_t  subsustem_id;
  uint32_t  eROM_base_addr;
  uint8_t   capabilities_pointer;
  uint8_t   reserved[7];
  uint8_t   interrput_line;
  uint8_t   interrput_pin;
  uint8_t   min_grant;
  uint8_t   max_latency;
} pci_device_t;

/*
 * Initialize PCI
 */
void pci_init();

/*
 * Find device
 */
pci_device_t* pci_find_device(uint16_t vendor, uint16_t device);


#endif   /* _PCI_H */
