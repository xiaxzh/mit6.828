#include <kern/e1000.h>
#include <kern/pmap.h>
#include <inc/string.h>

#define E1000_STATUS   0x00008  /* Device Status - RO */
#define E1000_TDBAL    0x03800  /* TX Descriptor Base Address Low - RW */
#define E1000_TDBAH    0x03804  /* TX Descriptor Base Address High - RW */
#define E1000_TDLEN    0x03808  /* TX Descriptor Length - RW */
#define E1000_TDH      0x03810  /* TX Descriptor Head - RW */
#define E1000_TDT      0x03818  /* TX Descripotr Tail - RW */
#define E1000_TCTL     0x00400  /* TX Control - RW */
#define E1000_TIPG     0x00410  /* TX Inter-packet gap -RW */

#define E1000_RAL      0x05400  /* Receive Address Low- RW Array */
#define E1000_RAH      0x05404  /* Receive Address High- RW Array */
#define E1000_MTA      0x05200  /* Multicast Table Array - RW Array */
#define E1000_RDBAL    0x02800  /* RX Descriptor Base Address Low - RW */
#define E1000_RDBAH    0x02804  /* RX Descriptor Base Address High - RW */
#define E1000_RDLEN    0x02808  /* RX Descriptor Length - RW */
#define E1000_RDH      0x02810  /* RX Descriptor Head - RW */
#define E1000_RDT      0x02818  /* RX Descriptor Tail - RW */
#define E1000_RCTL     0x00100  /* RX Control - RW */


/* Transmit control */
#define E1000_TCTL_EN     0x00000002    /* enable tx */
#define E1000_TCTL_PSP    0x00000008    /* pad short packets */
#define E1000_TCTL_CT     0x00000ff0    /* collision threshold */
#define E1000_TCTL_COLD   0x003ff000    /* collision distance */

/* Transmit Descriptor bit definition */
#define E1000_TXD_CMD_RS     0x08 /* Report Status */
#define E1000_TXD_CMD_EOP    0x01 /* End of Packet */
#define E1000_TXD_STAT_DD    0x00000001 /* Descriptor Done */

/* Receive control */
#define E1000_RCTL_EN             0x00000002    /* enable */
#define E1000_RCTL_BAM            0x00008000    /* broadcast enable */
#define E1000_RCTL_SZ_2048        0x00000000    /* rx buffer size 2048 */
#define E1000_RCTL_SECRC          0x04000000    /* Strip Ethernet CRC */

/* Receive Descriptor bit definitions */
#define E1000_RXD_STAT_DD       0x01    /* Descriptor Done */

/* Receive Address */
#define E1000_RAH_AV  0x80000000        /* Receive descriptor valid */

volatile uint32_t *e1000;

#define TX_DESC_SIZE 32
#define RX_DESC_SIZE 128

// 为transmit descriptor array配置内存
struct tx_desc tx_desc_array[TX_DESC_SIZE] __attribute__ ((aligned (16)));
char tx_pbuf[TX_DESC_SIZE][TX_PACKET_SIZE];

// 为receive descriptor array配置内存
struct rx_desc rx_desc_array[RX_DESC_SIZE] __attribute__ ((aligned (16)));
char rx_pbuf[RX_DESC_SIZE][RX_PACKET_SIZE];

// LAB 6: Your driver code here

static void
e1000w(uint32_t index, uint32_t value)
{
    e1000[index/4] = value;
    e1000[E1000_STATUS/4]; // wait for write to finish, by reading
}

static uint32_t
e1000r(uint32_t index)
{
    return e1000[index/4];
}

void
e1000_tx_init()
{
    // 初始化所有tx_desc
    for (uint32_t i = 0; i < TX_DESC_SIZE; ++i) {
        tx_desc_array[i].addr = PADDR(tx_pbuf[i]);
        tx_desc_array[i].status = E1000_TXD_STAT_DD;
        tx_desc_array[i].cmd = E1000_TXD_CMD_RS | E1000_TXD_CMD_EOP;
    }
    // 将tx_desc_array的地址写入TDBAL, TDBAH为0
    e1000w(E1000_TDBAL, PADDR(tx_desc_array));
    e1000w(E1000_TDBAH, 0);
    
    // 将tx_desc_array的长度in bytes写入TDLEN
    e1000w(E1000_TDLEN, sizeof(struct tx_desc)*TX_DESC_SIZE);

    // 初始化(0b) TDH和TDT指针
    e1000w(E1000_TDH, 0);
    e1000w(E1000_TDT, 0);

    // 初始化TCTL
    e1000w(E1000_TCTL, E1000_TCTL_EN | E1000_TCTL_PSP
                        | (E1000_TCTL_CT & (0x10 << 4))
                        | (E1000_TCTL_COLD & (0x40 << 12)));

    // 初始化TIPG
    e1000w(E1000_TIPG, 10 | (8 << 10) | (12 << 20));
}

void
e1000_rx_init()
{
    // 初始化所有rx_desc
    for (uint32_t i = 0; i < RX_DESC_SIZE; ++i) {
        rx_desc_array[i].addr = PADDR(rx_pbuf[i]);
    }

    // 将MAC地址52:54:00:12:34:56写入RAL和RAH
    e1000w(E1000_RAL, 0x12005452);
    e1000w(E1000_RAH, 0x5634 | E1000_RAH_AV);

    // 初始化MTA为0b
    e1000w(E1000_MTA, 0);

    // 将rx_desc_array的地址写入RDBAL，RDBAH为0
    e1000w(E1000_RDBAL, PADDR(rx_desc_array));
    e1000w(E1000_RDBAH, 0);

    // 将rx_desc_array的长度in bytes写入RDLEN
    e1000w(E1000_RDLEN, sizeof(struct rx_desc)*RX_DESC_SIZE);

    // 初始化 RDH和RDT指针
    e1000w(E1000_RDH, 0);
    e1000w(E1000_RDT, RX_DESC_SIZE-1); // 注意，RDT需要指向最后一个

    // 初始化RCTL，EN(1b)、BAM(1b)、BSIZE(2048)、SECRC(1b)
    e1000w(E1000_RCTL, E1000_RCTL_EN | E1000_RCTL_BAM
                        | E1000_RCTL_SZ_2048 | E1000_RCTL_SECRC);

}

int
pci_e1000_attach(struct pci_func *pcif)
{
	pci_func_enable(pcif);
    e1000 = mmio_map_region(pcif->reg_base[0], pcif->reg_size[0]);
    // uint32_t status = e1000r(E1000_STATUS);
	// cprintf("%x\n", status);
    e1000_tx_init();
    e1000_rx_init();
    return 0;
}

// transmit a packet
int
e1000_tx(void* addr, int length)
{
    // 检查发送队列是否已满
    uint32_t tdt = e1000r(E1000_TDT);
    if (tx_desc_array[tdt].status & E1000_TXD_STAT_DD) {
        // 将数据包拷贝至buffer的对应位置
        memmove(tx_pbuf[tdt], addr, length);
        // 更新对应的描述符
        tx_desc_array[tdt].addr = PADDR(tx_pbuf[tdt]);
        // cprintf("this packet tx_pbuf: 0x%x %d\n", tx_pbuf[tdt], tdt);
        tx_desc_array[tdt].length = length;
        tx_desc_array[tdt].status &= (~E1000_TXD_STAT_DD);
        // cprintf("before update, %d %d\n", e1000r(E1000_TDT), tdt);
        // 更新E1000上的TDT
        tdt = (tdt+1)%TX_DESC_SIZE;
        e1000w(E1000_TDT, tdt);
        // cprintf("after update, %d %d\n", e1000r(E1000_TDT), tdt);
    } else { // array is full
        return -1;
    }
    return 0;
}

// receive a packet
int
e1000_rx(void* addr, int* length)
{
    static int next = 0;
    // 检查接收队列是否为空
    if (rx_desc_array[next].status & E1000_RXD_STAT_DD) {
        *length = rx_desc_array[next].length;
        memmove(addr, rx_pbuf[next], *length);
        rx_desc_array[next].status &= (~E1000_RXD_STAT_DD);
        next = (next+1)%RX_DESC_SIZE;
        uint32_t rdt = e1000r(E1000_RDT);
        e1000w(E1000_RDT, (rdt+1)%RX_DESC_SIZE);
    } else {
        return -1;
    }
    return 0;
}
