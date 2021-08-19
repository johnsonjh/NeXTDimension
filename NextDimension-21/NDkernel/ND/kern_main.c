/*
 * kern_main:
 *
 *	The main for the ND kernel.  This ultimately becomes process 0.
 *
 *	On entry, the processor is running with interupts enabled, in supervisor mode,
 *	on the kernel interrupt stack allocated in locore.s.  The MMU has been
 *	configured with a 1-1 virtual to physical mapping.
 *
 *	Process 0 is constructed by hand on entry.
 *	Following this, the message process and pager are spawned.
 */
#include <sys/types.h>
#include <sys/map.h>
#include <i860/param.h>
#include <i860/vm_param.h>
#include "i860/proc.h"

extern struct proc proc[];
extern struct proc *P;		/* Global ptr to current process */

extern void Messages();
extern void PageOutDaemon();
extern void main();

kern_main(sp)
	vm_offset_t sp;
{
	proc[0].p_addr = sp;		/* Current top of stack */
	proc[0].p_stat = SRUN;		/* Current state is runnable */
	proc[0].p_flag = SLOAD|SSYS;	/* VM configured, phys mem wired down. */
	proc[0].p_pri = PSCHED;		/* Set scheduler priority */
	proc[0].p_dirbase = get_dirbase();	/* MMU context info for process */
	P = &proc[0];			/* Current process is 0 */
	
	
	SpawnProc( Messages, 0 );	/* Create a message process */
	SpawnProc( PageOutDaemon, 0 );	/* Create a pageout process */

	SpawnUserProc( main, 0 );	/* Start the user's main running. */

	Scheduler();			/* Never returns */
}

