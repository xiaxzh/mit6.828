#ifndef JOS_KERN_E1000_H
#define JOS_KERN_E1000_H

#define E1000_DEV_ID_82540EM            0x100E

#define TX_PACKET_SIZE  2048
#define RX_PACKET_SIZE  2048

#include "pci.h"

struct tx_desc {
    uint64_t addr;
    uint16_t length;
    uint8_t cso;
    uint8_t cmd;
    uint8_t status;
    uint8_t css;
    uint16_t special;
} __attribute__ ((packed));

struct rx_desc {
    uint64_t addr;
    uint16_t length;
    uint16_t pk_checksum;
    uint8_t status;
    uint8_t errors;
    uint16_t special;
} __attribute__ ((packed));

int pci_e1000_attach(struct pci_func *pcif);
int e1000_tx(void* addr, int length);
int e1000_rx(void* addr, int* length);


#endif  // SOL >= 6
