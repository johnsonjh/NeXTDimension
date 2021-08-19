#include "ND_patches.h"			/* Must be FIRST! */
#import <kern/xpr.h>
#import <sys/buf.h>
#import <sys/param.h>
#import <vm/vm_param.h>
#import <sys/callout.h>
#import <sys/kernel.h>
#import <kern/sched_prim.h>
#include <kern/mach_user_internal.h>
#include <nextdev/slot.h>
#include <next/pcb.h>
#import <sys/message.h>
#include <ND/NDmsg.h>
#include "ND_var.h"

static void shortpause( ND_var_t *, NDQueue * );
/*
 * Establish a multiprocessor lock against the DPS task and other
 * bus master boards.  We use Leslie Lamport's algorithm.  See "A Fast Mutual
 * Exclusion Algorithm", Report #5, DEC Systems Research Centre.
 *
 * We don't use the usual Mach multiprocessor locks because
 *
 *	a)	They were built with NCPUS = 1
 *	b)	We need to reliably lock in a heterogenous multiprocessor
 *		environment.
 *	c)	An atomic read-modify-write cycle may not be possible over the 
 *		NextBus with the next turn of NBIC chips.
 *	d)	The ND bus implementation doesn't support the i860 LOCK/UNLOCK opcodes!
 *
 * This locking algorithm should do very well under the circumstances.
 * Contention should be quite infrequent.  When we do get contention, we sleep
 * for a short time, rather than spin.  This avoids a deadlock condition with 
 * Display PostScript, should it be in the middle of an attempt to lock the interface
 * when we are scheduled in.
 */
 void
ND_LamportLock( ND_var_t *sp, NDQueue *q )
{
	while ( 1 )
	{
	    q->Lock_b[LOCK_NUM] = TRUE;
	    q->Lock_x = LOCK_NUM;
	    if ( q->Lock_y != LOCK_UNDEF )
	    {
		q->Lock_b[LOCK_NUM] = FALSE;
		while ( q->Lock_y != LOCK_UNDEF )	/* Spin on lock */
			shortpause( sp, q );
		continue;
	    }
	    q->Lock_y = LOCK_NUM;
	    if ( q->Lock_x != LOCK_NUM )
	    {
	    	int j;
	    	q->Lock_b[LOCK_NUM] = FALSE;
		for ( j = 0; j < MAX_LOCK_NUM; ++j )	/* Spin on other locks */
		{
		    while ( q->Lock_b[j] != FALSE )
			    shortpause(sp, q);
		}
		if ( q->Lock_y != LOCK_NUM )
		{
		    while ( q->Lock_y != LOCK_UNDEF )	/* Spin on lock */
			    shortpause(sp, q);
		    continue;
		}	
	    }
	    return;	/* We have established a lock */
	}
}

 void
ND_LamportSpinLock( NDQueue *q )
{
	while ( 1 )
	{
	    q->Lock_b[LOCK_NUM] = TRUE;
	    q->Lock_x = LOCK_NUM;
	    if ( q->Lock_y != LOCK_UNDEF )
	    {
		q->Lock_b[LOCK_NUM] = FALSE;
		while ( q->Lock_y != LOCK_UNDEF )	/* Spin on lock */
			;
		continue;
	    }
	    q->Lock_y = LOCK_NUM;
	    if ( q->Lock_x != LOCK_NUM )
	    {
	    	int j;
	    	q->Lock_b[LOCK_NUM] = FALSE;
		for ( j = 0; j < MAX_LOCK_NUM; ++j )	/* Spin on other locks */
		{
		    while ( q->Lock_b[j] != FALSE )
			    ;
		}
		if ( q->Lock_y != LOCK_NUM )
		{
		    while ( q->Lock_y != LOCK_UNDEF )	/* Spin on lock */
			    ;
		    continue;
		}	
	    }
	    return;	/* We have established a lock */
	}
}


 static void
shortpause( ND_var_t *sp, NDQueue *q )
{
	simple_lock( &sp->lock );
	timeout(thread_wakeup, (int)q, 5);	/* Suspend rather than spin */
	thread_sleep((int)q, &sp->lock, TRUE);
	simple_unlock( &sp->lock );
}




