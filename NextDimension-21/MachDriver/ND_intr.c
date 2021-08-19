/*
 * Copyright 1989 NeXT, Inc.
 *
 * Prototype NextDimension kernel server.
 *
 * This server implements a trivial Mach driver that monitors NextBus devices for
 * interrupts and dispatches a message on receipt of an interrupt.
 */
#include "ND_patches.h"			/* Must be FIRST! */
#include <kernserv/kern_server_types.h>	/* Caution: imports mach_user_internal.h */
#include <nextdev/slot.h>
#include <next/pcb.h>
#include <next/scr.h>
#include <next/spl.h>
#include <vm/vm_kern.h>
#include <sys/callout.h>
#include "ND_var.h"

/*
 * prototypes for kernel functions
 */
void *kalloc(int);
void kfree( void *, int);
void thread_wakeup(void *);
void assert_wait(void *, boolean_t);
void thread_block();
void timeout(void (*f)(), int, int );
void untimeout(void (*f)(), int);
kern_return_t port_alloc(task_t, port_t *);
task_t task_self();
thread_t kernel_thread(task_t, void (*)());



extern ND_var_t ND_var[];

extern kern_server_t instance;	

/*
 * Routine called within the driver to re-enable interrupts from a device.
 */
 void
ND_arm_interrupts( ND_var_t * sp)
{
	int s;
	
	s = spl_ND();
	simple_lock(&sp->flags_lock);
	sp->flags &= ~ND_INTR_PENDING;
	simple_unlock(&sp->flags_lock);
	*ND_mask_register(sp - ND_var) = SLOT_MASK_BIT; /* Permit interrupts */
	splx(s);
}
/*
 * Routine called within the driver to disable interrupts from a device.
 */
 void
ND_disarm_interrupts( ND_var_t * sp)
{
	*ND_mask_register(sp - ND_var) = 0; /* Block interrupts */
}

/*
 * This routine is called at hardware interrupt level when an external
 * card interrupts.  All this routine does is clear the interrupt and post
 * a wakeup to any paused service threads.
 */
int ND_intr(void)
{
	volatile int slotid;
	ND_var_t *sp;
	kern_return_t r;
	
	/*
	 * Find the slot that interrupted us (if any).
	 */
	/*
	 * Figure out if we really handle this interrupt.  Interrupts at level
	 * 5 are polled, so it's not just as easy as simply forwarding the 
	 * interrupt on.  We claim all interrupts associated with slots 2, 4, and 6,
	 * or our logical units 1, 2, and 3, if attached.
	 */
	for (slotid = 1; slotid < SLOTCOUNT; slotid++)
	{
	    sp = &ND_var[slotid];
	    if (sp->present && (*ND_intr_register(slotid)&SLOT_INTR_BIT) != 0)
	    {
#if defined(PROTOTYPE)
		*ADDR_NDP_CSR(sp) = *ADDR_NDP_CSR(sp) & ~NDCSR_INTCPU;
#else
		*ADDR_NDP_CSR1(sp) = 0;	/* Clear the host interrupt register. */
#endif
		break;
	    }
	}
	/*
	 * If we couldn't handle the interrupt, leave now.
	 */
	if (slotid == SLOTCOUNT)
		return FALSE;			/* Poll code should try next handler */

	if ( ! sp->attached )		/* No service threads for this board */
		return TRUE;		/* Nothing we can do with intr! */

	simple_lock( &sp->flags_lock );
	sp->flags |= ND_INTR_PENDING;
	simple_unlock( &sp->flags_lock );
	/* 
	 * Awaken any sleepers.  Thread_wakeup messes with the IPL,
	 * so we have to be careful here, and use a callout to
	 * post the wakeup!  (And people wonder why we don't do real
	 * time stuff....)
	 */
#if 0
	if ( (sp->flags & ND_REPLY_THREAD_PAUSED) != 0 )
	{
	    r = kern_serv_callout(  &instance,
			    thread_wakeup,
			    (void *)&sp->reply_thread );
	    if ( r != KERN_SUCCESS )
	    	printf( "NextDimension: kern_serv_callout(1) returns %d\n",r );
	}
	if ( (sp->flags & ND_SERVER_THREAD_PAUSED) != 0 )
	{
	    r = kern_serv_callout(  &instance,
			    thread_wakeup,
			    (void *)&sp->server_thread );
	    if ( r != KERN_SUCCESS )
	    	printf( "NextDimension: kern_serv_callout(2) returns %d\n",r );
	}
#endif
	if ( (sp->flags & ND_REPLY_THREAD_PAUSED) != 0 )
	{
	    softint_sched(CALLOUT_PRI_SOFTINT1,
		    (int (*)())thread_wakeup,
		    (caddr_t)&sp->reply_thread);
	}
	if ( (sp->flags & ND_SERVER_THREAD_PAUSED) != 0 )
	{
	    softint_sched(CALLOUT_PRI_SOFTINT1,
		    (int (*)())thread_wakeup,
		    (caddr_t)&sp->server_thread);
	}

	return TRUE;	/* We fielded the interrupt. Poll code should stop. */
}





