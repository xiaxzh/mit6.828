#include "ns.h"
#include <inc/assert.h>

extern union Nsipc nsipcbuf;

void
output(envid_t ns_envid)
{
	binaryname = "ns_output";

	// LAB 6: Your code here:
	// 	- read a packet from the network server
	//	- send the packet to the device driver

	envid_t whom;
	uint32_t req;
	int perm;
	// struct jif_pkt* pg = (struct jif_pkt*) 0x0ffff000;

	while (1) {
		req = ipc_recv(&whom, &nsipcbuf, &perm);

		if (req != NSREQ_OUTPUT || whom != ns_envid) {
			cprintf("Invalid request from %08x: not Network Server or wrong req type\n",
				whom);
			continue;
		}

		if (!(perm & PTE_P)) {
			cprintf("Invalid request from %08x: no argument\n",
				whom);
			continue;
		}

		int count = 0;
		// cprintf("recv packet : %s\n", pg->jp_data);
		while (sys_e1000_try_tx(nsipcbuf.pkt.jp_data, nsipcbuf.pkt.jp_len) < 0) {
			sys_yield();
		}
	}
}
