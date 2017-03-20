/*
 * PCI
 */

#include "types.h"
#include "hw86.h"
#include "pci.h"
#include "ulib/ulib.h"

#define MAX_PCI_DEVICE 32

static uint pci_count = 0;
struct PCI_DEVICE pci_table[MAX_PCI_DEVICE];

/*
 * Get configuration address base
 */
static uint32_t pci_dev(uchar bus, uint8_t slot, uint8_t func)
{
	return (uint32_t)
    (((ul_t)bus<<16) | ((ul_t)slot<<11) | ((ul_t)func<<8));
}

/*
 * Read config
 */
static uint32_t pci_read_config(uint32_t pci_dev, uint8_t offset)
{
  uint32_t data = 0x80000000L | pci_dev | (ul_t)(offset & 0xFC);
	outl(data, PCI_CONFIG_ADDR_PORT);
	return inl(PCI_CONFIG_DATA_PORT);
}

/*
 * Read config (word)
 */
static uint16_t pci_read_config_word(uint32_t pci_dev, uint8_t offset)
{
	union {
		uint32_t ret_i;
		uint16_t ret_s[2];
	} ret;

	ret.ret_i = pci_read_config(pci_dev, offset);

	if((offset & 3) == 0) {
		return ret.ret_s[0];
	} else if((offset & 3) == 2) {
		return ret.ret_s[1];
  }

  return 0x77FF;
}

/*
 * Scan devices in bus 0
 */
void pci_init()
{
	uint8_t bus = 0;
	uint8_t slot;
	uint8_t func;
	uint32_t tmp_pci_dev;
	uint16_t vendor_id;
	uint i;

	/* Initialize PCI table */
	memset(pci_table, 0, sizeof(pci_table));

	/* Scan bus 0 devices */
  pci_count = 0;
	for(slot=0; slot<32; slot++) {
		for(func=0; func<8; func++) {
      uint i;
      uint32_t* p;
			tmp_pci_dev = pci_dev(bus, slot, func);
			vendor_id = pci_read_config_word(tmp_pci_dev, 0);

			/* Discard unused */
			if(vendor_id == 0xFFFF) {
				continue;
      }

			/* Read device */
			pci_table[pci_count].dev = tmp_pci_dev;
			p = (uint32_t *)&(pci_table[pci_count].vendor_id);

			for(i=0; i<64; i+=4) {
				*p++ = pci_read_config(tmp_pci_dev, i);
			}

			pci_count++;
			if(pci_count >= MAX_PCI_DEVICE) {
				debugstr("There are too many devices, skip\n\r");
				return;
			}
		}
	}

	/* Print debug info */
  debugstr("PCI initialized\n\r");
	for(i=0; i<pci_count; i++) {
		debugstr("dev:%X  vendor:%x  device:%x\n", pci_table[i].dev,
			pci_table[i].vendor_id, pci_table[i].device_id);
	}
}

/*
 * Find device in bus 0
 * Previous initialization is required
 */
struct PCI_DEVICE* pci_find_device(uint16_t vendor, uint16_t device)
{
	struct PCI_DEVICE* pci = pci_table;
  uint i;
	for(i=0; i<pci_count; i++) {
		if(vendor==pci->vendor_id && device==pci->device_id) {
			return pci;
    }
		pci++;
	}
	return 0;
}
