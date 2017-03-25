/*
 * PCI
 */

#include "types.h"
#include "hw86.h"
#include "pci.h"
#include "ulib/ulib.h"

#define PCI_CONFIG_ADDR_PORT 0xCF8
#define PCI_CONFIG_DATA_PORT 0xCFC

#define MAX_PCI_DEVICE 16
static uint pci_count = 0;
struct PCI_DEVICE pci_devices[MAX_PCI_DEVICE];

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
 * Scan devices in bus 0
 */
void pci_init()
{
	uint8_t bus = 0;
	uint8_t slot;
	uint8_t func;
	uint i;

	/* Initialize PCI table */
	memset(pci_devices, 0, sizeof(pci_devices));

	/* Scan bus 0 devices */
  pci_count = 0;
	for(slot=0; slot<32; slot++) {
		for(func=0; func<8; func++) {
      uint i;
      uint32_t* p;
			uint32_t pci_dev_addr = (uint32_t)
		    (((ul_t)bus<<16) | ((ul_t)slot<<11) | ((ul_t)func<<8));

			uint16_t vendor_id = pci_read_config(pci_dev_addr, 0)&0xFFFF;

			/* Discard unused */
			if(vendor_id == 0xFFFF) {
				continue;
      }

			/* Read device */
			p = (uint32_t *)&pci_devices[pci_count];

			for(i=0; i<sizeof(struct PCI_DEVICE); i+=4) {
				*p++ = pci_read_config(pci_dev_addr, i);
			}

			pci_count++;
			if(pci_count >= MAX_PCI_DEVICE) {
				debugstr("There are unlisted PCI devices\n\r");
				return;
			}
		}
	}

	/* Print debug info */
  debugstr("PCI initialized\n\r");
	for(i=0; i<pci_count; i++) {
		debugstr("vendor:%x  device:%x\n",
			pci_devices[i].vendor_id, pci_devices[i].device_id);
	}
}

/*
 * Find device in bus 0
 * Previous initialization is required
 */
struct PCI_DEVICE* pci_find_device(uint16_t vendor, uint16_t device)
{
  uint i;
	for(i=0; i<pci_count; i++) {
		if(vendor==pci_devices[i].vendor_id &&
			device==pci_devices[i].device_id) {
			return &pci_devices[i];
    }
	}
	return 0;
}
