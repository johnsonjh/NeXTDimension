#include <setjmp.h>
#include <sys/map.h>
#include <sys/errno.h>
#include <sys/kern_return.h>
#include <i860/param.h>
#include <vm/vm_param.h>
#include "i860/proc.h"

extern int curpri, runin, runrun;
extern struct proc proc[];
extern struct proc *P;	/* The global current process */
extern volatile int intrs;

void Sleep( int, int );
void Wakeup( int );
/*
 * idle:
 *
 *	Wait for an interrupt to have occurred, and then return.
 */
 static void
idle()
{
	extern volatile int intrs;
	register int i, j, s;
	
	i = intrs;
	s = spl0();
	flush();	/* Since we have time to kill, update phys memory. */
	j = 1000000;	/* After a while, look anyway. */
	while ( i == intrs && --j)
		;
	splx(s);
}
/*
 * SpawnProc:
 *
 *	Construct a new lightweight process with it's entry point as specified
 *	by the argument to SpawnProc.  Allocate a page from the coremap to act as
 *	a stack for the process.
 */
SpawnProc( func, data )
    void (*func)();
    void *data;
{
	extern struct map *coremap;
	register struct proc *p;
	unsigned page;
	vm_offset_t newstack;
	extern void CoInit();
	
	/* Find a free entry in the process table */
	for ( p = proc; p < &proc[NPROC]; ++p )
	{
		if ( p->p_stat == SFREE )
			break;
	}
	if ( p >= &proc[NPROC] )
	{
		P->p_error = EAGAIN;	/* No more processes */
		return( -1 );
	}
	/* Copy the current process state into the new process */
	*p = *P;	/* Inherit full state, runnable, dirbase, etc... */
	p->p_pid = p - proc;
	
	/* Get a new page for a stack. REPLACE WHEN PAGER IS READY. */
	if ( (page = rmalloc( coremap, 1 )) == 0 )
	{
		P->p_error = ENOMEM;	/* No more memory */
		p->p_stat = SFREE;
		return( -1 );
	}
	newstack = i860_ptob(page);
	
	/*
	 * Save the state for the new process, and add in the stack and entry point.
	 * A stack frame is faked up so as to terminate any stack crawl done by the
	 * debugger.  CoInit will complete the stack frame.
	 *
	 * The entry pc for the new coroutine and one argument are loaded into
	 * register save locations in p_rsav.  The address of an assembly language 
	 * glue routine is stuffed into the r1 save location in p_rsav, so that
	 * when Switch() does a longjmp() on p_rsav, it will jump to the beginning
	 * of the glue code.  This code will set up the actual call into the
	 * coroutine.
	 */
	setjmp( p->p_rsav );
	newstack += i860_PGBYTES;		/* kernel stack top */
	p->p_addr = newstack - 16;		/* Trap stack top (16 for CoInit())*/
	
	p->p_rsav[JB_SP] = newstack;		/* New stack pointer */
	p->p_rsav[JB_FP] = newstack - 8;	/* New frame pointer */
	
	p->p_rsav[JB_R1] = (int) CoInit;	/* longjmp() 'returns' to CoInit */
	
	/* CoInit expects to find the func address and an arg in r14 and r15. */
	p->p_rsav[JB_R14] = (int) func;		/* coroutine function */
	p->p_rsav[JB_R15] = (int) data;		/* Arg to function */
}

/*
 * SpawnUserProc:
 *
 *	Construct a new lightweight process with it's entry point as specified
 *	by the argument to SpawnProc.  Allocate a page from the coremap to act as
 *	an interrupt stack for the process.
 *
 *	Allocate a block of VM to act as a user mode stack.
 */
SpawnUserProc( func, data )
    void (*func)();
    void *data;
{
	extern struct map *coremap;
	register struct proc *p;
	unsigned page;
	vm_offset_t newstack;
	vm_offset_t userstack;
	kern_return_t r;
	extern void CoInit();

	/* Find a free entry in the process table */
	for ( p = proc; p < &proc[NPROC]; ++p )
	{
		if ( p->p_stat == SFREE )
			break;
	}
	if ( p >= &proc[NPROC] )
	{
		P->p_error = EAGAIN;	/* No more processes */
		return( -1 );
	}
	/* Copy the current process state into the new process */
	*p = *P;	/* Inherit full state, runnable, dirbase, etc... */
	p->p_pid = p - proc;
	p->p_stat = SIDL;	/* Under construction... */
	
	/*
	 * Get the user stack. This may sleep!
	 * First, try to place the stack in the user stack arena.
	 * If that fails, try anywhere.
	 */
	userstack = PROC_VM_STACK_ARENA + (PROC_VM_STACK_ALLOC * p->p_pid);
	r = vm_allocate(task_self(), &userstack, PROC_VM_STACK_ALLOC, 0);
	if ( r != KERN_SUCCESS )
	{
	    r = vm_allocate(task_self(), &userstack, PROC_VM_STACK_ALLOC, 1);
	    if ( r != KERN_SUCCESS )
		    return -1;
	}
	p->p_vmstack = userstack;
	/* Get a new page for a stack. REPLACE WHEN PAGER IS READY. */
	if ( (page = rmalloc( coremap, 1 )) == 0 )
	{
		P->p_error = ENOMEM;	/* No more memory */
		p->p_stat = SFREE;
		return( -1 );
	}
	newstack = i860_ptob(page);
	
	/*
	 * Save the state for the new process, and add in the stack and entry point.
	 * A stack frame is faked up so as to terminate any stack crawl done by the
	 * debugger.  CoInit will complete the stack frame.
	 *
	 * The entry pc for the new coroutine and one argument are loaded into
	 * register save locations in p_rsav.  The address of an assembly language 
	 * glue routine is stuffed into the r1 save location in p_rsav, so that
	 * when Switch() does a longjmp() on p_rsav, it will jump to the beginning
	 * of the glue code.  This code will set up the actual call into the
	 * coroutine.
	 */
	setjmp( p->p_rsav );
	newstack += i860_PGBYTES;		/* kernel stack top */
	p->p_addr = newstack - 16;		/* Trap stack top (16 for CoInit())*/
	userstack = p->p_vmstack + PROC_VM_STACK_ALLOC - 16;

	p->p_rsav[JB_SP] = userstack;		/* New stack pointer */
	p->p_rsav[JB_FP] = userstack - 8;	/* New frame pointer */
	
	p->p_rsav[JB_R1] = (int) CoInit;	/* longjmp() 'returns' to CoInit */
	
	/* CoInit expects to find the func address and an arg in r14 and r15. */
	p->p_rsav[JB_R14] = (int) func;		/* coroutine function */
	p->p_rsav[JB_R15] = (int) data;		/* Arg to function */
	p->p_stat = SRUN;	/* Process is now runnable! */

}

/*
 * DestroyProc:
 *
 *	Tear down a process, possibly our own!
 *
 *	Free the process stack and the proc structure.
 *	Call Switch() to find a new process to be run if we zapped ourselves.
 */
 void
DestroyProc( p )
    struct proc *p;
{
	extern struct map *coremap;
	
	rmfree( coremap, 1, i860_btop(round_page(p->p_addr)-i860_PGBYTES) );
	if ( p->p_vmstack )
	    vm_deallocate( task_self(), p->p_vmstack, PROC_VM_STACK_ALLOC );
	p->p_stat = SFREE;	/* No longer runnable. */
	if ( p == P )		/* Did we zap ourselves? */
		Switch();		/* Find a runnable process. */
}

/*
 * Scheduler:
 *
 *	See if an unloaded context wants to run.
 *	If so, look around for resources to hold the context.
 *	Possibly throw other contexts out until there is enough room.
 *	Load in the context to be run.
 *	Repeat....
 *
 *	Believe it or not, this code really does scheduling...  All the real work
 *	happens in Sleep(), for co-operative multitasking.
 *
 *	This is currently a no-op, as we are only running one context.
 *	Things do change, though....
 */
Scheduler()
{
	while ( 1 )
		Sleep( (int)&runin, PSWAP );
}

 int 
CurrentPriority()
{
	return(P->p_pri);
}
/*
 * Mark a process as wanting to go to sleep for a while, and call the 
 * Switch() function to reschedule the processor.  Nice sleeps give the 
 * scheduler a chance to run and maybe shuffle the process queue.  When the scheduler
 * sleeps, it takes the rude branch to avoid disturbing runin and doing a false 
 * wakeup.
 */
 void
Sleep( chan, pri )
    int chan;
    int pri;
{
	struct proc *p = P;
	int s;
	
	if ( pri >= 0 )	/* Nice polite wait for resources */
	{
		s = splsched();
		p->p_wchan = chan;
		p->p_stat = SWAIT;
		p->p_pri = pri;
		splx(s);
		if ( runin )	/* Scheduler waiting? Give it guaranteed 1st shot */
		{
			runin = 0;
			Wakeup( (int) &runin );
		}
		Switch();
	}
	else		/* Rude wait for something that better happen soon... */
	{
		s = splsched();
		p->p_wchan = chan;
		p->p_stat = SWAIT;
		p->p_pri = pri;
		splx(s);
		Switch();
	}
	return;
}

/*
 * Run through the process list, marking all processes sleeping on 'chan' as runnable.
 */
 void
Wakeup( chan )
    int chan;
{
	struct proc *p = proc;
	int i;
	
	for ( i = 0; i < NPROC; ++i )
	{
		if ( p->p_wchan == chan )
			setrun( p );
		++p;
	}
}

/*
 * setrun:
 *
 *	Set the process 'running'.  
 */
setrun( p )
    struct proc *p;
{
	p->p_wchan = 0;	/* Not sleeping anymore.  make Wakeup() ignore this proc. */
	p->p_stat = SRUN;	/* It's runnable! */
	if ( p->p_pri < curpri ) /* Ruder than current proc?  Force reschedule */
		++runrun;
}

/*
 * setpri:
 *
 *	Set the priority for the selected process.  We could do adaptive prioritization
 *	here, if we choose.  For now, we're going to implement an evil absolute
 *	priority scheme in hopes of getting deterministic real time behavior.
 *	This therefore does almost nothing.
 */
setpri( p )
    struct proc * p;
{
	if ( p->p_pri < curpri )	/* More important than current process? */
		++runrun;	/* Need to reschedule! */
}

/*
 * Switch:
 *
 *	Reschedule the processor.  This is an act in three parts:
 *
 *	1) Save the current process state.
 *	2) Switch to the scheduler stack (proc[0]) and grovel through the procs
 *	   looking for the new winner.  If none is runnable, go idle() and 
 *	   wait for an interrupt to give us something to do.
 *	3) Switch to the winner's kernel stack and resume it.
 *
 *	All of this flipping about puts some odd constraints on the data used
 *	within the function.  Automatic (stack) variables are clearly out here!
 *	The register variables used have been checked in the compiler output to
 *	be OK.
 */
Switch()
{
	static struct proc *p;
	
	if ( p == (struct proc *) 0 )
		p = proc;	/* Reset to head of process list */

	/* Save the current process state */
	switch ( setjmp( P->p_rsav ) )
	{
	    case 0:
		/* We just saved the old state. Switch to the new stack and state. */
		splsched();	/* Cleared during longjmp() */
		P = &proc[0];
		longjmp( proc[0].p_rsav, 1 ); /* Reload the stack, frame, etc. */
		break;
	    case 1:
	    	break;	/* We just called longjmp(), above, selecting process 0 */
	    case 2:
	    	return 1; /* We just called longjmp() after picking a new process. */
	}
	
	/*
	 * At this point, we are in process 0.
	 *
	 * Search for the highest priority (lowest number) runnable process.
	 * We do some slight of hand to go through all processes starting from 
	 * the last runnable one.
	 */
	 while ( 1 )
	 {
	 	register int i;
		register int pri = PMIN;
		register struct proc *pp;
		
		runrun = 0; /* We are going to reschedule to highest priority proc */
		pp = p;
		p = (struct proc *) 0;
		
		i = NPROC;	/* Count through the proc list ONCE. */
		do
		{
		    if ( ++pp >= &proc[NPROC] )	/* Next process... */
			    pp = proc;	/* Special case: wrap around */
		    if (pp->p_stat == SRUN && (pp->p_flag & SLOAD) && pp->p_pri < pri)
		    {	/* Runnable, VM set up, more urgent priority. A winner! */
		    	p = pp;
			pri = pp->p_pri;
		    }
		}
		while ( --i );
		
		if ( p == (struct proc *) 0 )	/* Nothing to run? */
		{
			idle();		/* Wait for an interrupt and maybe some work */
			p = pp;		/* set up proc pointer. */
			continue;	/* Interrupt gave us something to do? */
		}
		curpri = pri;		/* Set global idea of new proc priority. */
		break;
	 }
	/*
	 * The third part of our operation:  Switch to the new process.
	 */
	splsched();			/* Cleared during longjmp() */

	/* Need to do a full VM context switch? */
	if ( trunc_page(p->p_dirbase) != trunc_page(get_dirbase()) )
		new_dirbase( p->p_dirbase );
	P = p;
	longjmp( p->p_rsav, 2 );	/* Reload the stack, frame, and whatnot. */
	
	/* NOT REACHED */
}

