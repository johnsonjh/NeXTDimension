#include <c.h>
#include <ND/NDmsg.h>


 void
ND_LamportLockInit( NDQueue *q )
{
	bzero( (char *)q, sizeof (NDQueue) );
	q->Lock_x = LOCK_UNDEF;
	q->Lock_y = LOCK_UNDEF;
}

/*
 * If Lock_y is set, some processor holds the lock.
 * The Lock_b array need not be examined.
 */
 int
ND_LamportLockSet( NDQueue *q )
{
	if ( q->Lock_y != LOCK_UNDEF )
		return 1;
	return 0;
}

/*
 * Establish a multiprocessor lock against the DPS task and other
 * bus master boards.  We use Leslie Lamport's algorithm.  See "A Fast Mutual
 * Exclusion Algorithm", Report #5, DEC Systems Research Centre.
 *
 *	1)	We need to reliably lock in a heterogenous multiprocessor
 *		environment.
 *	2)	An atomic read-modify-write cycle is not be possible over the 
 *		NextBus with the NBIC chips.
 *	3)	The ND bus implementation doesn't support the i860
 *		LOCK/UNLOCK opcodes!
 */
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
		{
		}
		continue;
	    }
	    q->Lock_y = LOCK_NUM;
	    if ( q->Lock_x != LOCK_NUM )
	    {
	    	volatile int j;	/* Force array element access to be volatile */

	    	q->Lock_b[LOCK_NUM] = FALSE;
		for ( j = 0; j < MAX_LOCK_NUM; ++j ) /* Spin on other locks */
		{
		    while ( q->Lock_b[j] != FALSE )
		    {
		    }
		}
		if ( q->Lock_y != LOCK_NUM )
		{
		    while ( q->Lock_y != LOCK_UNDEF )	/* Spin on lock */
		    {
		    }
		    continue;
		}	
	    }
	    return;	/* We have established a lock */
	}
}

 void
ND_LamportUnlock(NDQueue *q)
{
	q->Lock_y = LOCK_UNDEF;
	q->Lock_b[LOCK_NUM] = FALSE;
}


