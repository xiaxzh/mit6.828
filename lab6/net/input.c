#include "ns.h"
#include <kern/e1000.h>

extern union Nsipc nsipcbuf;

void
sleep(int msec)
{
	unsigned int now = sys_time_msec();
	unsigned int end = now + msec;

	if (msec < 0 || end < now)
		panic("sys_time_msec: msec should not < 0 or overflow\n");
	
	while (sys_time_msec() < end)
		sys_yield();
}

void
input(envid_t ns_envid)
{
	binaryname = "ns_input";

	// LAB 6: Your code here:
	// 	- read a packet from the device driver
	//	- send it to the network server
	// Hint: When you IPC a page to the network server, it will be
	// reading from it for a while, so don't immediately receive
	// another packet in to the same physical page.

	int length;
	char rev_buf[RX_PACKET_SIZE];
	
	while (1) {
		while (sys_e1000_try_rx(rev_buf, &length) < 0)
			sys_yield();
		memcpy(nsipcbuf.pkt.jp_data, rev_buf, length);
		nsipcbuf.pkt.jp_len = length;

		ipc_send(ns_envid, NSREQ_INPUT, &nsipcbuf, PTE_P | PTE_U);
		sleep(50);
	}
}