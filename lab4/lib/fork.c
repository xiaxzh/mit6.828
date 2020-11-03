// implement fork from user space

#include <inc/string.h>
#include <inc/lib.h>

// PTE_COW marks copy-on-write page table entries.
// It is one of the bits explicitly allocated to user processes (PTE_AVAIL).
#define PTE_COW		0x800

//
// Custom page fault handler - if faulting page is copy-on-write,
// map in our own private writable copy.
//
static void
pgfault(struct UTrapframe *utf)
{
	void *addr = (void *) utf->utf_fault_va;
	uint32_t err = utf->utf_err;
	int r;

	// Check that the faulting access was (1) a write, and (2) to a
	// copy-on-write page.  If not, panic.
	// Hint:
	//   Use the read-only page table mappings at uvpt
	//   (see <inc/memlayout.h>).

	// LAB 4: Your code here.

	// 做到这里有疑惑，代码运行到这里是处于用户态
	// 可以访问内核部分的地址吗？(访问页表)
	// 经过一番回顾（学习）发现这个内核在设计时，
	// 把PDE的权限设置过了，用户有权限修改和访问。
	// 1. 如果访问的行为不是写，或者va所在的页面不存在或不是COW页
	// 那么panic
	if ((err&FEC_WR) != FEC_WR || (uvpd[PDX(addr)]&PTE_P) != PTE_P
			|| (uvpt[PGNUM(addr)]&PTE_P) != PTE_P || (uvpt[PGNUM(addr)] & PTE_COW) != PTE_COW)
		panic("panic at lib pgfault for not write access or not a COW page\n");

	// Allocate a new page, map it at a temporary location (PFTEMP),
	// copy the data from the old page to the new page, then move the new
	// page to the old page's address.
	// Hint:
	//   You should make three system calls.

	// LAB 4: Your code here.
	// panic("pgfault not implemented");

	// 根据COW的定义，如果某一个进程执行更改页面的操作
	// 那么这个进程将配置新的内存页，并将COW页的内容拷贝
	// 到新配置的内存页中，并创建进程独占的地址映射。

	// 这里不要使用thisenv，在子进程被创建，没有修改thisenv，但触发了缺页中断时
	// 使用thisenv会带来bug，所以这里都使用sys_getenvid。
	envid_t curenvid = sys_getenvid();

	// 配置一个新的页面，并将其映射至暂存区
	r = sys_page_alloc(curenvid, (void*)PFTEMP, PTE_U | PTE_W | PTE_P);
	if (r < 0)
		panic("panic at lib pgfault for sys_page_alloc\n");

	// 将COW页的内容拷贝至暂存区
	memmove((void*)PFTEMP, ROUNDDOWN(addr, PGSIZE), PGSIZE);

	// 销毁addr地址上的映射
	r = sys_page_unmap(curenvid, ROUNDDOWN(addr, PGSIZE));
	if (r < 0)
		panic("panic at lib pgfault for sys_page_unmap\n");
	
	// 建立addr地址上新的映射
	r = sys_page_map(curenvid, (void*)PFTEMP,
			curenvid, ROUNDDOWN(addr, PGSIZE), PTE_U | PTE_W | PTE_P);
	if (r < 0)
		panic("panic at lib pgfault for sys_page_map\n");

	// 销毁暂存区的映射
	r = sys_page_unmap(curenvid, (void*)PFTEMP);
	if (r < 0)
		panic("panic at lib pgfault for sys_page_unmap\n");

}

//
// Map our virtual page pn (address pn*PGSIZE) into the target envid
// at the same virtual address.  If the page is writable or copy-on-write,
// the new mapping must be created copy-on-write, and then our mapping must be
// marked copy-on-write as well.  (Exercise: Why do we need to mark ours
// copy-on-write again if it was already copy-on-write at the beginning of
// this function?)
//
// Returns: 0 on success, < 0 on error.
// It is also OK to panic on error.
//
/*
 * 与指定进程，共享物理内存页
 * 即为指定进程创建映射关系，
 * 如果原映射是只读的，那么新创建的映射也是只读的
 * 如果原映射可写，那么需要将原映射和新映射都设置为COW
 */
static int
duppage(envid_t envid, unsigned pn)
{
	int r;

	// LAB 4: Your code here.
	// panic("duppage not implemented");
	void* addr;
	pte_t pte;
	int newPerm = PTE_P | PTE_U;

	envid_t curenvid = sys_getenvid();

	addr = (void*) (pn * PGSIZE);
	pte = uvpt[pn];
	if ((pte&PTE_W) == PTE_W || (pte&PTE_COW) == PTE_COW)
		newPerm |= PTE_COW;
	
	r = sys_page_map(curenvid, addr, envid, addr, newPerm);
	if (r < 0)
		return r;
	
	// Ques: 为什么需要对原映射再做一次映射呢？
	// Ans: 为了更改原映射的权限（COW）
	if ((newPerm&PTE_COW) == PTE_COW) {
		r = sys_page_map(curenvid, addr, curenvid, addr, newPerm);
		if (r < 0)
			return r;
	}

	return 0;
}

//
// User-level fork with copy-on-write.
// Set up our page fault handler appropriately.
// Create a child.
// Copy our address space and page fault handler setup to the child.
// Then mark the child as runnable and return.
//
// Returns: child's envid to the parent, 0 to the child, < 0 on error.
// It is also OK to panic on error.
//
// Hint:
//   Use uvpd, uvpt, and duppage.
//   Remember to fix "thisenv" in the child process.
//   Neither user exception stack should ever be marked copy-on-write,
//   so you must allocate a new page for the child's user exception stack.
//
envid_t
fork(void)
{
	// LAB 4: Your code here.
	// panic("fork not implemented");
	envid_t envid;

	set_pgfault_handler(pgfault);
	envid = sys_exofork();
	if (envid < 0) {
		panic("panic at lib fork for envid < 0\n");
	} else if (envid > 0) {
		// 父进程
		// exofork只为子进程创建了中断帧
		// 将子进程的状态设置为不可运行
		// 并且没有为子进程分配页表
		// 待父进程完成了COW的配置以后，
		// 将子进程的状态设置为可运行
		size_t i, j, pn;
		for (i = PDX(UTEXT); i < PDX(UXSTACKTOP); i++) {
			if ((uvpd[i]&PTE_P) == PTE_P) {
				for (j = 0; j < NPDENTRIES; j++) {
					pn = PGNUM(PGADDR(i, j, 0));
					if (pn == PGNUM(UXSTACKTOP - PGSIZE))
						break;
					if ((uvpt[pn]&PTE_P) == PTE_P)
						duppage(envid, pn);
				}
			}
		}

		// 为子进程配置异常栈
		int r;
		r = sys_page_alloc(envid, (void*) UXSTACKTOP-PGSIZE, PTE_P | PTE_W | PTE_U);
		if (r < 0)
			panic("panic at lib fork for sys_page_alloc\n");
		
		// 为子进程设置pgfault_upcall
		extern void _pgfault_upcall(void);

		r = sys_env_set_pgfault_upcall(envid, _pgfault_upcall);
		if (r < 0)
			panic("panic at lib fork for sys_env_set_pgfault_upcall\n");

		r = sys_env_set_status(envid, ENV_RUNNABLE);
		if (r  < 0)
			panic("panic at lib fork for sys_env_set_status\n");

		return envid;
	} else {
		// envid = 0 子进程
		thisenv = &envs[ENVX(sys_getenvid())];
		return 0;
	}

}

// Challenge!
int
sfork(void)
{
	panic("sfork not implemented");
	return -E_INVAL;
}
