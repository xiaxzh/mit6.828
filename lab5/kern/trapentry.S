/* See COPYRIGHT for copyright information. */

#include <inc/mmu.h>
#include <inc/memlayout.h>
#include <inc/trap.h>

#include <kern/picirq.h>


###################################################################
# exceptions/interrupts
###################################################################

/* TRAPHANDLER defines a globally-visible function for handling a trap.
 * It pushes a trap number onto the stack, then jumps to _alltraps.
 * Use TRAPHANDLER for traps where the CPU automatically pushes an error code.
 *
 * You shouldn't call a TRAPHANDLER function from C, but you may
 * need to _declare_ one in C (for instance, to get a function pointer
 * during IDT setup).  You can declare the function with
 *   void NAME();
 * where NAME is the argument passed to TRAPHANDLER.
 */
#define TRAPHANDLER(name, num)						\
	.globl name;		/* define global symbol for 'name' */	\
	.type name, @function;	/* symbol type is function */		\
	.align 2;		/* align function definition */		\
	name:			/* function starts here */		\
	pushl $(num);							\
	jmp _alltraps

/* Use TRAPHANDLER_NOEC for traps where the CPU doesn't push an error code.
 * It pushes a 0 in place of the error code, so the trap frame has the same
 * format in either case.
 */
#define TRAPHANDLER_NOEC(name, num)					\
	.globl name;							\
	.type name, @function;						\
	.align 2;							\
	name:								\
	pushl $0;							\
	pushl $(num);							\
	jmp _alltraps

.text

/*
 * Lab 3: Your code here for generating entry points for the different traps.
 */

/*
 * Read http://wiki.osdev.org/Exceptions
 * and 386 manual for details of exceptions
 */
TRAPHANDLER_NOEC(divide_handle, T_DIVIDE)
TRAPHANDLER_NOEC(debug_handle, T_DEBUG)
TRAPHANDLER_NOEC(nmi_handle, T_NMI)
TRAPHANDLER_NOEC(brkpt_handle, T_BRKPT)
TRAPHANDLER_NOEC(oflow_handle, T_OFLOW)
TRAPHANDLER_NOEC(bound_handle, T_BOUND)
TRAPHANDLER_NOEC(illop_handle, T_ILLOP)
TRAPHANDLER_NOEC(device_handle, T_DEVICE)
TRAPHANDLER(dblflt_handle, T_DBLFLT)
TRAPHANDLER(tss_handle, T_TSS)
TRAPHANDLER(segnp_handle, T_SEGNP)
TRAPHANDLER(stack_handle, T_STACK)
TRAPHANDLER(gpflt_handle, T_GPFLT)
TRAPHANDLER(pgflt_handle, T_PGFLT)
TRAPHANDLER_NOEC(fperr_handle, T_FPERR)
TRAPHANDLER(align_handle, T_ALIGN)
TRAPHANDLER_NOEC(mchk_handle, T_MCHK)
TRAPHANDLER_NOEC(simderr_handle, T_SIMDERR)
TRAPHANDLER_NOEC(syscall_handle, T_SYSCALL)

/* 外部中断 */
TRAPHANDLER_NOEC(IRQ0_handle, IRQ_OFFSET+0)
TRAPHANDLER_NOEC(IRQ1_handle, IRQ_OFFSET+1)
TRAPHANDLER_NOEC(IRQ2_handle, IRQ_OFFSET+2)
TRAPHANDLER_NOEC(IRQ3_handle, IRQ_OFFSET+3)
TRAPHANDLER_NOEC(IRQ4_handle, IRQ_OFFSET+4)
TRAPHANDLER_NOEC(IRQ5_handle, IRQ_OFFSET+5)
TRAPHANDLER_NOEC(IRQ6_handle, IRQ_OFFSET+6)
TRAPHANDLER_NOEC(IRQ7_handle, IRQ_OFFSET+7)
TRAPHANDLER_NOEC(IRQ8_handle, IRQ_OFFSET+8)
TRAPHANDLER_NOEC(IRQ9_handle, IRQ_OFFSET+9)
TRAPHANDLER_NOEC(IRQ10_handle, IRQ_OFFSET+10)
TRAPHANDLER_NOEC(IRQ11_handle, IRQ_OFFSET+11)
TRAPHANDLER_NOEC(IRQ12_handle, IRQ_OFFSET+12)
TRAPHANDLER_NOEC(IRQ13_handle, IRQ_OFFSET+13)
TRAPHANDLER_NOEC(IRQ14_handle, IRQ_OFFSET+14)
TRAPHANDLER_NOEC(IRQ15_handle, IRQ_OFFSET+15)


/*
 * Lab 3: Your code here for _alltraps
 */


_alltraps:
	/* push values to make the stack look like a struct Trapframe */
	pushl %ds;
	pushl %es;
	pushal;

	/* load GD_KD into %ds and %es */
	movw $GD_KD, %ax;
	movw %ax, %ds;
	movw %ax, %es;
	
	/* pushl %esp to pass a pointer to the Trapframe as an argument to trap() */
	pushl %esp;
	
	/* call trap (can trap ever return?) */
	call trap;


