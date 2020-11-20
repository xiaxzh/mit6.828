#include <inc/mmu.h>
#include <inc/x86.h>
#include <inc/assert.h>

#include <kern/pmap.h>
#include <kern/trap.h>
#include <kern/console.h>
#include <kern/monitor.h>
#include <kern/env.h>
#include <kern/syscall.h>
#include <kern/sched.h>
#include <kern/kclock.h>
#include <kern/picirq.h>
#include <kern/cpu.h>
#include <kern/spinlock.h>
#include <kern/time.h>

static struct Taskstate ts;

/* For debugging, so print_trapframe can distinguish between printing
 * a saved trapframe and printing the current trapframe and print some
 * additional information in the latter case.
 */
static struct Trapframe *last_tf;

/* Interrupt descriptor table.  (Must be built at run time because
 * shifted function addresses can't be represented in relocation records.)
 */
struct Gatedesc idt[256] = { { 0 } };
struct Pseudodesc idt_pd = {
	sizeof(idt) - 1, (uint32_t) idt
};


static const char *trapname(int trapno)
{
	static const char * const excnames[] = {
		"Divide error",
		"Debug",
		"Non-Maskable Interrupt",
		"Breakpoint",
		"Overflow",
		"BOUND Range Exceeded",
		"Invalid Opcode",
		"Device Not Available",
		"Double Fault",
		"Coprocessor Segment Overrun",
		"Invalid TSS",
		"Segment Not Present",
		"Stack Fault",
		"General Protection",
		"Page Fault",
		"(unknown trap)",
		"x87 FPU Floating-Point Error",
		"Alignment Check",
		"Machine-Check",
		"SIMD Floating-Point Exception"
	};

	if (trapno < ARRAY_SIZE(excnames))
		return excnames[trapno];
	if (trapno == T_SYSCALL)
		return "System call";
	if (trapno >= IRQ_OFFSET && trapno < IRQ_OFFSET + 16)
		return "Hardware Interrupt";
	return "(unknown trap)";
}

// 定义在trapentry.S中
extern void divide_handle();
extern void debug_handle();
extern void nmi_handle();
extern void brkpt_handle();
extern void oflow_handle();
extern void bound_handle();
extern void illop_handle();
extern void device_handle();
extern void dblflt_handle();
extern void tss_handle();
extern void segnp_handle();
extern void stack_handle();
extern void gpflt_handle();
extern void pgflt_handle();
extern void fperr_handle();
extern void align_handle();
extern void mchk_handle();
extern void simderr_handle();
extern void syscall_handle();
extern void IRQ0_handle();
extern void IRQ1_handle();
extern void IRQ2_handle();
extern void IRQ3_handle();
extern void IRQ4_handle();
extern void IRQ5_handle();
extern void IRQ6_handle();
extern void IRQ7_handle();
extern void IRQ8_handle();
extern void IRQ9_handle();
extern void IRQ10_handle();
extern void IRQ11_handle();
extern void IRQ12_handle();
extern void IRQ13_handle();
extern void IRQ14_handle();
extern void IRQ15_handle();


void
trap_init(void)
{
	extern struct Segdesc gdt[];

	// LAB 3: Your code here.
	// 职责：建立中断向量表
	SETGATE(idt[T_DIVIDE], 0, GD_KT, divide_handle, 0);
	SETGATE(idt[T_DEBUG], 0, GD_KT, debug_handle, 0);
	SETGATE(idt[T_NMI], 0, GD_KT, nmi_handle, 0);
	SETGATE(idt[T_BRKPT], 0, GD_KT, brkpt_handle, 3); // 课程中将breakpoint异常中的DPL设置为3
	SETGATE(idt[T_OFLOW], 0, GD_KT, oflow_handle, 0);
	SETGATE(idt[T_BOUND], 0, GD_KT, bound_handle, 0);
	SETGATE(idt[T_ILLOP], 0, GD_KT, illop_handle, 0);
	SETGATE(idt[T_DEVICE], 0, GD_KT, device_handle, 0);
	SETGATE(idt[T_DBLFLT], 0, GD_KT, dblflt_handle, 0);
	SETGATE(idt[T_TSS], 0, GD_KT, tss_handle, 0);
	SETGATE(idt[T_SEGNP], 0, GD_KT, segnp_handle, 0);
	SETGATE(idt[T_STACK], 0, GD_KT, stack_handle, 0);
	SETGATE(idt[T_GPFLT], 0, GD_KT, gpflt_handle, 0);
	SETGATE(idt[T_PGFLT], 0, GD_KT, pgflt_handle, 0);
	SETGATE(idt[T_FPERR], 0, GD_KT, fperr_handle, 0);
	SETGATE(idt[T_ALIGN], 0, GD_KT, align_handle, 0);
	SETGATE(idt[T_MCHK], 0, GD_KT, mchk_handle, 0);
	SETGATE(idt[T_SIMDERR], 0, GD_KT, simderr_handle, 0);
	SETGATE(idt[T_SYSCALL], 0, GD_KT, syscall_handle, 3);

	// 外部中断
	SETGATE(idt[IRQ_OFFSET+0], 0, GD_KT, IRQ0_handle, 3);
	SETGATE(idt[IRQ_OFFSET+1], 0, GD_KT, IRQ1_handle, 3);
	SETGATE(idt[IRQ_OFFSET+2], 0, GD_KT, IRQ2_handle, 3);
	SETGATE(idt[IRQ_OFFSET+3], 0, GD_KT, IRQ3_handle, 3);
	SETGATE(idt[IRQ_OFFSET+4], 0, GD_KT, IRQ4_handle, 3);
	SETGATE(idt[IRQ_OFFSET+5], 0, GD_KT, IRQ5_handle, 3);
	SETGATE(idt[IRQ_OFFSET+6], 0, GD_KT, IRQ6_handle, 3);
	SETGATE(idt[IRQ_OFFSET+7], 0, GD_KT, IRQ7_handle, 3);
	SETGATE(idt[IRQ_OFFSET+8], 0, GD_KT, IRQ8_handle, 3);
	SETGATE(idt[IRQ_OFFSET+9], 0, GD_KT, IRQ9_handle, 3);
	SETGATE(idt[IRQ_OFFSET+10], 0, GD_KT, IRQ10_handle, 3);
	SETGATE(idt[IRQ_OFFSET+11], 0, GD_KT, IRQ11_handle, 3);
	SETGATE(idt[IRQ_OFFSET+12], 0, GD_KT, IRQ12_handle, 3);
	SETGATE(idt[IRQ_OFFSET+13], 0, GD_KT, IRQ13_handle, 3);
	SETGATE(idt[IRQ_OFFSET+14], 0, GD_KT, IRQ14_handle, 3);
	SETGATE(idt[IRQ_OFFSET+15], 0, GD_KT, IRQ15_handle, 3);



	// Per-CPU setup 
	trap_init_percpu();
}

// Initialize and load the per-CPU TSS and IDT
void
trap_init_percpu(void)
{
	// The example code here sets up the Task State Segment (TSS) and
	// the TSS descriptor for CPU 0. But it is incorrect if we are
	// running on other CPUs because each CPU has its own kernel stack.
	// Fix the code so that it works for all CPUs.
	//
	// Hints:
	//   - The macro "thiscpu" always refers to the current CPU's
	//     struct CpuInfo;
	//   - The ID of the current CPU is given by cpunum() or
	//     thiscpu->cpu_id;
	//   - Use "thiscpu->cpu_ts" as the TSS for the current CPU,
	//     rather than the global "ts" variable;
	//   - Use gdt[(GD_TSS0 >> 3) + i] for CPU i's TSS descriptor;
	//   - You mapped the per-CPU kernel stacks in mem_init_mp()
	//   - Initialize cpu_ts.ts_iomb to prevent unauthorized environments
	//     from doing IO (0 is not the correct value!)
	//
	// ltr sets a 'busy' flag in the TSS selector, so if you
	// accidentally load the same TSS on more than one CPU, you'll
	// get a triple fault.  If you set up an individual CPU's TSS
	// wrong, you may not get a fault until you try to return from
	// user space on that CPU.
	//
	// LAB 4: Your code here:

	// Setup a TSS so that we get the right stack
	// when we trap to the kernel.
	size_t i = thiscpu->cpu_id;
	thiscpu->cpu_ts.ts_esp0 = KSTACKTOP - i * (KSTKSIZE + KSTKGAP);
	thiscpu->cpu_ts.ts_ss0 = GD_KD;
	thiscpu->cpu_ts.ts_iomb = sizeof(struct Taskstate);

	// Initialize the TSS slot of the gdt.
	gdt[(GD_TSS0 >> 3) + i] = SEG16(STS_T32A, (uint32_t) (&thiscpu->cpu_ts),
					sizeof(struct Taskstate) - 1, 0);

	gdt[(GD_TSS0 >> 3) + i].sd_s = 0;

	// Load the TSS selector (like other segment selectors, the
	// bottom three bits are special; we leave them 0)
	ltr(GD_TSS0 + (i << 3));

	// Load the IDT
	lidt(&idt_pd);
}

void
print_trapframe(struct Trapframe *tf)
{
	cprintf("TRAP frame at %p from CPU %d\n", tf, cpunum());
	print_regs(&tf->tf_regs);
	cprintf("  es   0x----%04x\n", tf->tf_es);
	cprintf("  ds   0x----%04x\n", tf->tf_ds);
	cprintf("  trap 0x%08x %s\n", tf->tf_trapno, trapname(tf->tf_trapno));
	// If this trap was a page fault that just happened
	// (so %cr2 is meaningful), print the faulting linear address.
	if (tf == last_tf && tf->tf_trapno == T_PGFLT)
		cprintf("  cr2  0x%08x\n", rcr2());
	cprintf("  err  0x%08x", tf->tf_err);
	// For page faults, print decoded fault error code:
	// U/K=fault occurred in user/kernel mode
	// W/R=a write/read caused the fault
	// PR=a protection violation caused the fault (NP=page not present).
	if (tf->tf_trapno == T_PGFLT)
		cprintf(" [%s, %s, %s]\n",
			tf->tf_err & 4 ? "user" : "kernel",
			tf->tf_err & 2 ? "write" : "read",
			tf->tf_err & 1 ? "protection" : "not-present");
	else
		cprintf("\n");
	cprintf("  eip  0x%08x\n", tf->tf_eip);
	cprintf("  cs   0x----%04x\n", tf->tf_cs);
	cprintf("  flag 0x%08x\n", tf->tf_eflags);
	if ((tf->tf_cs & 3) != 0) {
		cprintf("  esp  0x%08x\n", tf->tf_esp);
		cprintf("  ss   0x----%04x\n", tf->tf_ss);
	}
}

void
print_regs(struct PushRegs *regs)
{
	cprintf("  edi  0x%08x\n", regs->reg_edi);
	cprintf("  esi  0x%08x\n", regs->reg_esi);
	cprintf("  ebp  0x%08x\n", regs->reg_ebp);
	cprintf("  oesp 0x%08x\n", regs->reg_oesp);
	cprintf("  ebx  0x%08x\n", regs->reg_ebx);
	cprintf("  edx  0x%08x\n", regs->reg_edx);
	cprintf("  ecx  0x%08x\n", regs->reg_ecx);
	cprintf("  eax  0x%08x\n", regs->reg_eax);
}

void
trap_dispatch(struct Trapframe *tf)
{
	// Handle processor exceptions.
	// LAB 3: Your code here.

	if (tf->tf_trapno == T_PGFLT) {
		// 如果是page fault
		page_fault_handler(tf);
		return;
	} else if (tf->tf_trapno == T_BRKPT) {
		// 如果是breakpoint
		monitor(tf);
		return;
	} else if (tf->tf_trapno == T_SYSCALL) {
		// The system call number will go in %eax, 
		// and the arguments (up to five of them) will go in %edx, %ecx, %ebx, %edi, and %esi, respectively. 
		int32_t ret = syscall(tf->tf_regs.reg_eax,
			tf->tf_regs.reg_edx, tf->tf_regs.reg_ecx,
			tf->tf_regs.reg_ebx, tf->tf_regs.reg_edi,
			tf->tf_regs.reg_esi);
		tf->tf_regs.reg_eax = ret;
		return;
	}

	// Handle spurious interrupts
	// The hardware sometimes raises these because of noise on the
	// IRQ line or other reasons. We don't care.
	if (tf->tf_trapno == IRQ_OFFSET + IRQ_SPURIOUS) {
		cprintf("Spurious interrupt on irq 7\n");
		print_trapframe(tf);
		return;
	}

	// Handle clock interrupts. Don't forget to acknowledge the
	// interrupt using lapic_eoi() before calling the scheduler!
	// LAB 4: Your code here.
	if (tf->tf_trapno == IRQ_OFFSET + IRQ_TIMER) {
		lapic_eoi();
		// Add time tick increment to clock interrupts.
		// Be careful! In multiprocessors, clock interrupts are
		// triggered on every CPU.
		// LAB 6: Your code here.
		time_tick();
		sched_yield();
		return;
	}

	


	// Handle keyboard and serial interrupts.
	// LAB 5: Your code here.
	if (tf->tf_trapno == IRQ_OFFSET + IRQ_KBD) {
		kbd_intr();
		return;
	}

	if (tf->tf_trapno == IRQ_OFFSET + IRQ_SERIAL) {
		serial_intr();
		return;
	}

	
	// Unexpected trap: The user process or the kernel has a bug.
	print_trapframe(tf);
	if (tf->tf_cs == GD_KT)
		panic("unhandled trap in kernel"); 
	else {
		env_destroy(curenv);
		return;
	}
}

void
trap(struct Trapframe *tf)
{
	// The environment may have set DF and some versions
	// of GCC rely on DF being clear
	asm volatile("cld" ::: "cc");

	// Halt the CPU if some other CPU has called panic()
	extern char *panicstr;
	if (panicstr)
		asm volatile("hlt");

	// Re-acqurie the big kernel lock if we were halted in
	// sched_yield()
	if (xchg(&thiscpu->cpu_status, CPU_STARTED) == CPU_HALTED)
		lock_kernel();
	// Check that interrupts are disabled.  If this assertion
	// fails, DO NOT be tempted to fix it by inserting a "cli" in
	// the interrupt path.
	assert(!(read_eflags() & FL_IF));

	if ((tf->tf_cs & 3) == 3) {
		// Trapped from user mode.
		// Acquire the big kernel lock before doing any
		// serious kernel work.
		// LAB 4: Your code here.
		assert(curenv);
		lock_kernel();

		// Garbage collect if current enviroment is a zombie
		if (curenv->env_status == ENV_DYING) {
			env_free(curenv);
			curenv = NULL;
			sched_yield();
		}

		// Copy trap frame (which is currently on the stack)
		// into 'curenv->env_tf', so that running the environment
		// will restart at the trap point.
		curenv->env_tf = *tf;
		// The trapframe on the stack should be ignored from here on.
		tf = &curenv->env_tf;
	}

	// Record that tf is the last real trapframe so
	// print_trapframe can print some additional information.
	last_tf = tf;

	// Dispatch based on what type of trap occurred
	trap_dispatch(tf);

	// If we made it to this point, then no other environment was
	// scheduled, so we should return to the current environment
	// if doing so makes sense.
	if (curenv && curenv->env_status == ENV_RUNNING)
		env_run(curenv);
	else
		sched_yield();
}


void
page_fault_handler(struct Trapframe *tf)
{
	uint32_t fault_va;

	// Read processor's CR2 register to find the faulting address
	fault_va = rcr2();

	// Handle kernel-mode page faults.

	// LAB 3: Your code here.
	if (tf->tf_cs == GD_KT) {
		panic("pg fault in kernel mode, fault in %08x\n", fault_va);
	}

	// We've already handled kernel-mode exceptions, so if we get here,
	// the page fault happened in user mode.

	// Call the environment's page fault upcall, if one exists.  Set up a
	// page fault stack frame on the user exception stack (below
	// UXSTACKTOP), then branch to curenv->env_pgfault_upcall.
	//
	// The page fault upcall might cause another page fault, in which case
	// we branch to the page fault upcall recursively, pushing another
	// page fault stack frame on top of the user exception stack.
	//
	// It is convenient for our code which returns from a page fault
	// (lib/pfentry.S) to have one word of scratch space at the top of the
	// trap-time stack; it allows us to more easily restore the eip/esp. In
	// the non-recursive case, we don't have to worry about this because
	// the top of the regular user stack is free.  In the recursive case,
	// this means we have to leave an extra word between the current top of
	// the exception stack and the new stack frame because the exception
	// stack _is_ the trap-time stack.
	//
	// If there's no page fault upcall, the environment didn't allocate a
	// page for its exception stack or can't write to it, or the exception
	// stack overflows, then destroy the environment that caused the fault.
	// Note that the grade script assumes you will first check for the page
	// fault upcall and print the "user fault va" message below if there is
	// none.  The remaining three checks can be combined into a single test.
	//
	// Hints:
	//   user_mem_assert() and env_run() are useful here.
	//   To change what the user environment runs, modify 'curenv->env_tf'
	//   (the 'tf' variable points at 'curenv->env_tf').

	// LAB 4: Your code here.

	if (curenv->env_pgfault_upcall != NULL) {
		// 1. 先判断是否已经切换到异常栈了
		// 如果没有，那么需要先执行栈切换
		// 本来想用ss来判断是否执行了栈切换
		// 但课程中的UTrapframe没有包含ss，
		// 也就是说ss将在整个用户的pg handler
		// 中将不发生改变，因此不能使用ss来判断
		// 因此使用esp来判断。
		struct UTrapframe* utf;
		if (tf->tf_esp >= UXSTACKTOP || tf->tf_esp < UXSTACKTOP-PGSIZE)
			utf = (struct UTrapframe*) (UXSTACKTOP - sizeof(struct UTrapframe));
		else
			utf = (struct UTrapframe*) (tf->tf_esp-4 - sizeof(struct UTrapframe));
		user_mem_assert(curenv, (void *)utf, sizeof(struct UTrapframe), PTE_U | PTE_W);
		// 2. 将utf压入异常栈
		utf->utf_esp = tf->tf_esp;
		utf->utf_eflags = tf->tf_eflags;
		utf->utf_eip = tf->tf_eip;
		utf->utf_regs = tf->tf_regs;
		utf->utf_err = tf->tf_err;
		utf->utf_fault_va = fault_va;
		
		// 3. 执行curenv->env_pgfault_upcall
		tf->tf_eip = (uintptr_t) curenv->env_pgfault_upcall;
		tf->tf_esp = (uintptr_t) utf;
		env_run(curenv);
	}

	// Destroy the environment that caused the fault.
	cprintf("[%08x] user fault va %08x ip %08x\n",
		curenv->env_id, fault_va, tf->tf_eip);
	print_trapframe(tf);
	env_destroy(curenv);
}

void
break_point_handler(struct Trapframe *tf) {
	cprintf("Welcome to the JOS kernel monitor!\n");
	cprintf("Type 'help' for a list of commands.\n");

	if (tf != NULL)
		print_trapframe(tf);
	env_destroy(curenv);
}
