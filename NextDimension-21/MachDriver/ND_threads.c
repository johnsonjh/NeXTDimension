/*
 * Copyright 1989 NeXT, Inc.
 *
 * Prototype NextDimension kernel server.
 *
 * This module contains the code used to start and stop service threads.
 */
#include "ND_patches.h"			/* Must be FIRST! */
#include <nextdev/slot.h>
#include <next/pcb.h>
#include <next/scr.h>
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


/*
 * Destroy the service threads for this slot/board.
 */
 void
ND_stop_services( sp )
    ND_var_t *sp;
{
	int tries = 0;
	int s;
	
	s = spl_ND();
	ND_disarm_interrupts( sp );	
#if !defined(PROTOTYPE)
	*ADDR_NDP_CSR1(sp) = 0;			/* Clear interrupts from board. */
#endif /* PROTOTYPE */
	*ADDR_NDP_CSR(sp) = NDCSR_RESET860;	/* Halt the ND board i860 */
	
	splx(s);
	
	simple_lock( &sp->flags_lock );
	sp->flags |= (ND_STOP_SERVER_THREAD | ND_STOP_REPLY_THREAD);
	simple_unlock( &sp->flags_lock );
#if SOON
	lock_done( sp->lock );		/* Release any sleep locks */
#endif
	/*
	 * Destroy server port here, waking up the server thread.
	 */
	ND_deallocate_resources( sp );	/* Destroys kernel_port, the server port. */
	if ( (sp->flags & ND_SERVER_THREAD_RUNNING) != 0)
	{
		thread_wakeup(&sp->server_thread);			
		while((sp->flags & ND_SERVER_THREAD_RUNNING) != 0) {
			assert_wait((void *)&sp->flags, FALSE);
			thread_block();
		}
	}
	simple_lock( &sp->flags_lock );
	sp->flags |= ND_STOP_REPLY_THREAD;
	simple_unlock( &sp->flags_lock );
	if ( (sp->flags & ND_REPLY_THREAD_RUNNING) != 0)
	{
		thread_wakeup(&sp->reply_thread);			
		while((sp->flags & ND_REPLY_THREAD_RUNNING) != 0) {
			assert_wait((void *)&sp->flags, FALSE);
			thread_block();
		}
	}
}

/*
 * Start the service threads for this board/slot.
 */
 void
ND_start_services( sp )
    ND_var_t *sp;
{
	simple_lock( &sp->flags_lock );
	sp->flags &= ~(ND_STOP_REPLY_THREAD | ND_STOP_SERVER_THREAD);
	simple_unlock( &sp->flags_lock );
	/* Init service control flags */
	if ( (sp->flags & ND_SERVER_THREAD_RUNNING) == 0 )
	{
	    simple_lock( &sp->flags_lock );
	    sp->flags &= ~ND_SERVER_THREAD_RUNNING;
	    simple_unlock( &sp->flags_lock );
	    sp->server_thread = kernel_thread(current_task(), ND_msg_server_loop );
	    if ( (sp->flags & ND_SERVER_THREAD_RUNNING) == 0)
	    {
		assert_wait((void *)&sp->flags, FALSE); /* Wait for start */
		thread_block();
	    }
	}
	if ( (sp->flags & ND_REPLY_THREAD_RUNNING) == 0 )
	{
	    simple_lock( &sp->flags_lock );
	    sp->flags &= ~ND_REPLY_THREAD_RUNNING;
	    simple_unlock( &sp->flags_lock );
	    sp->reply_thread = kernel_thread(current_task(), ND_reply_server_loop );
	    if ( (sp->flags & ND_REPLY_THREAD_RUNNING) == 0)
	    {
		assert_wait((void *)&sp->flags, FALSE); /* Wait for start */
		thread_block();
	    }
	}
	/*
	 * Services are now available.  Clear any pending (leftover)
	 * interrupts and arm the interrupt handler.
	 */
#if defined(PROTOTYPE)
	*ADDR_NDP_CSR(sp) = *ADDR_NDP_CSR(sp) & ~NDCSR_INTCPU;
#else /* PROTOTYPE */
	*ADDR_NDP_CSR1(sp) = *ADDR_NDP_CSR1(sp) & ~NDCSR1_INTCPU;
#endif /* PROTOTYPE */
	ND_arm_interrupts( sp );
}

