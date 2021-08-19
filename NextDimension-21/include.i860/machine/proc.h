#ifndef _PROC_H_
#define _PROC_H_
/*
 * struct proc:
 *
 *	There is one of these for each 'process' that we run.  These are all really
 *	lightweight processes, sharing the same address space.  The proc structure is
 *	used to track priority and status for a given lightweight process, and also 
 *	holds a reference to the kernel stack in use by a process.
 *
 *	The entry p_addr MUST BE FIRST, as it is used by the trap handler to switch
 *	onto the kernel stack when another stack was active on trap. 
 */
#include <setjmp.h>
#include <machine/vm_types.h>

struct proc
{
	vm_offset_t	p_addr;	/* addr of top of proc kernel stack - Must be FIRST */
	jmp_buf		p_rsav;	/* Process switch data */
	jmp_buf		p_qsav;	/* Process fault recovery data */
	int 		p_stat;	/* Status: running, runnable, waiting, zombie.... */
	int 		p_flag;	/* Misc flags. */
	int		p_pid;	/* Process ID. */
	int 		p_pri;	/* Priority, negative is higher, Unix style. */
	int 		p_wchan;	/* What 'channel' is it waiting on? */
	int *		p_ar0;		/* base of kernel entry trap state on stack. */
	
	int *		p_arg;		/* System call args */
	int		p_rval1;	/* Sys call return value */
	int		p_rval2;	/* More of the same. */
	int		p_eosys;	/* How a system call ends. */
	
	unsigned long	p_dirbase;	/* Phys addr of 1st level PTE for context. */
					/* Entry for per-context pagers??? */
	vm_offset_t	p_recover;	/* Optional kernel fault handler for process */
	int		p_error;	/* last kernel error for this process */
	vm_offset_t	p_vmstack;	/* addr of top of proc VM stack */

};

/* Values for p_stat field */
#define SFREE		0	/* Free slot in process table. */
#define SSLEEP		1	/* Sleeping on high priority */
#define SWAIT		2	/* Sleeping on low priority */
#define SRUN		3	/* Running! */
#define SIDL		4	/* Idle: the process is under construction */
#define SZOMB		5	/* Zombie: the process is being terminated */
#define SSTOP		6	/* Stopped: the process is under debugger control */

/* values for p_flag field - I used the funky old names in spite of this VM system */
#define	SLOAD		0x00000001	/* VM mapping established, ready to run */
#define SSYS		0x00000002	/* Scheduling process */
#define SLOCK		0x00000004	/* Locked in core, not pageable */
#define STRC		0x00000008	/* Process has debugger attached */

/* Some nice canned values for the p_pri priority field */
#define PSWAP		-20		/* Really rude... */
#define PRUDE		-10		/* Rude internal system wait priority */
#define PZERO		0
#define PSCHED		10		/* Scheduler priority. */
#define PUSER		20
#define PIDLE		40
#define PMIN		2048

/* values for p_eosys */
#define NORMALRETURN	0
#define RESTARTSYS	1
#define JUSTRETURN	2

/* Process VM stack values */
#define PROC_VM_STACK_ALLOC	0x80000	/* 512 Kbytes */
#define PROC_VM_STACK_ARENA	0xE0000000
#endif
