#include "ND/NDlib.h"
#include <sys/message.h>
#include <sys/msg_type.h>
#include <libc.h>
#include <math.h>

/*
 * NDmsg_send:
 *
 *	Send a message as specified in the header.
 *	For the moment, we ignore the options and timeout, as we have no
 *	timer capability.  Every msg_send is a blocking send.
 */
 msg_return_t
NDmsg_send( msg_header_t *header, msg_option_t option, msg_timeout_t timeout )
{
	ND_var_p handle = (ND_var_p) 0;
	NDQueue *q;
	int size;
	int cnt;
	char *buf;
	char *src;
	port_t old_local_port;
	port_t old_remote_port;

	/*
	 * Route messages with out of line data or kernel ports via 
	 * the kernel ND driver, so the pager and translator get a shot at the data.
	 */
	if ( header->msg_simple != TRUE )
		return( msg_send( header, option, timeout ) );

	for ( cnt = 0; cnt < NUMHANDLES; ++cnt )
	{
		if ( _ND_Handles_[cnt] &&
		     _ND_Handles_[cnt]->kernel_port == header->msg_remote_port )
		{
			handle = _ND_Handles_[cnt];
			break;
		}
	}
	if ( handle == (ND_var_p) 0 )	/* Device not open, or inappropriate msg */
		return( msg_send( header, option, timeout ) );
	

	size = header->msg_size;

	buf = MSGBUF_HOST_TO_860(handle);
	q = &MSG_QUEUES(handle)->ToND;
	
	/*
	 * Check for adequate free space on the queue.
	 * If the message can't be dispatched immediately, send 
	 * it via the Mach message system, so SEND_NOTIFY and related 
	 * semantics work correctly.
	 */
	q = &MSG_QUEUES(handle)->ToND;
	if ( size >= (MSGBUF_SIZE - BYTES_ON_QUEUE(q)) )
		return( msg_send( header, option, timeout ) );
	/*
	 * Replace the task's ports with the matching kernel ports.
	 *
	 * We try to catch and cache any changes on the reply port here.
	 * This changes infrequently, if at all, and so is worth the
	 * overhead to keep up a current translation.  Just don't change
	 * reply ports too often!
	 */
	old_local_port = header->msg_local_port;
	old_remote_port = header->msg_remote_port;
	
	if ( header->msg_remote_port != handle->kernel_port )
		return( msg_send( header, option, timeout ) );
	if (  (header->msg_type & MSG_TYPE_RPC) != 0 )
	{
		if ( header->msg_local_port == handle->local_port )
			header->msg_local_port = handle->NDlocal_port;
		else
		{	/* Get the new mapping */
			handle->local_port = header->msg_local_port;
			/* This is a RPC, so is fairly expensive. */
			ND_PortTranslate( handle->dev_port, handle->local_port,
					&handle->NDlocal_port );
			header->msg_local_port = handle->NDlocal_port;
		}
	}
	header->msg_remote_port = handle->NDkernel_port;
	/*
	 *	Get ready to copy data onto the circular buffer/
	 */

	ND_LamportLock( q );				/* Set multiprocessor lock */
	src = (char *) header;

	/* Are we going to wrap around the buffer? */
	if ( (cnt = MSGBUF_SIZE - q->Head) < size )	/* Do copy in two pieces */
	{
		bcopy( src, &buf[q->Head], cnt );
		src += cnt;				/* Bump src to next byte */
		size -= cnt;
		
		bcopy( src, buf, size );		/* Copy 2nd half */
		q->Head = size;				/* Wrap head back around */
	}
	else						/* Do copy in one piece */
	{
		bcopy( src, &buf[q->Head], size );	/* Copy off queue */
		q->Head += size;			/* Bump up head pointer */
	}
	ND_INT860(handle);	/* Interrupt the i860 so it will get the message */
	
	ND_LamportUnlock( q );				/* Release multiproc lock */
	
	header->msg_local_port = old_local_port;	/* Restore the user's ports */
	header->msg_remote_port = old_remote_port;
	
	return SEND_SUCCESS;
}

/*
 * NDmsg_rpc:
 *
 *	Perform a message send followed by a message receive.
 */
 msg_return_t
NDmsg_rpc( msg_header_t *header, msg_option_t option, msg_size_t rcv_size,
		msg_timeout_t send_timeout, msg_timeout_t rcv_timeout )
{
	msg_return_t status;
	
	header->msg_type |= MSG_TYPE_RPC;	/* Just to make sure... */
	status = NDmsg_send( header, option, send_timeout );
	if ( status != SEND_SUCCESS )
		return( status );
	header->msg_size = rcv_size;
	return( msg_receive( header, option, rcv_timeout ) );
}


/*
 * Establish a multiprocessor lock against the kernel and other
 * bus master boards.  We use Leslie Lamport's algorithm.  See "A Fast Mutual
 * Exclusion Algorithm", Report #5, DEC Systems Research Centre.
 *
 * Contention should be quite infrequent.  If it occurs at all often, we might
 * want to throw in a usleep(20000) or so in the spin locks.
 *
 * ND_LamportUnlock() is defined as a macro in NDlib.h
 */
 void
ND_LamportLock( NDQueue *q )
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
		if ( q->Lock_y != LOCK_UNDEF )
		{
		    while ( q->Lock_y != LOCK_UNDEF )	/* Spin on lock */
			    ;
		    continue;
		}	
	    }
	    break;	/* We have established a lock */
	}
}

