/*
 * Copyright 1989 NeXT, Inc.
 *
 * NextDimension Device Driver
 *
 * This module contains the code which moves messages between Mach message ports and 
 * the circular buffers on the NextDimension board.
 */

#include "ND_patches.h"			/* Must be FIRST! */
#include <nextdev/slot.h>
#include <next/pcb.h>
#include <machine/spl.h>
#include <sys/param.h>
#include <ND/NDmsg.h>
#include "ND_var.h"
#include <sys/mig_errors.h>

/* Imported kernel functions. */
kern_return_t
    ND_vm_move(vm_map_t,vm_offset_t,vm_map_t,vm_offset_t, boolean_t,vm_offset_t*);


extern ND_var_t ND_var[SLOTCOUNT];
static boolean_t valid_msg( ND_var_t *, msg_header_t * );
void ND_writequeue( char *, NDQueue *, char *, int );

/*
 * ND_init_message_system:
 *
 *	Initialize the message system, including clearing the message queues
 *	and configuring the lamport lock structures.
 */
 void
ND_init_message_system( ND_var_t *sp )
{
	int s;
	if ( ! sp->present )
		return;
	s = spl_ND();
	bzero( MSG_QUEUES(sp), sizeof (NDMsgQueues) );	/* Clear the control structs */
	
	MSG_QUEUES(sp)->kernel_port = sp->kernel_port;	/* Set port info up. */
	MSG_QUEUES(sp)->debug_port = sp->debug_port;
	MSG_QUEUES(sp)->service_port = sp->service_port;
	MSG_QUEUES(sp)->service_replyport = sp->service_replyport;
	
	MSG_QUEUES(sp)->ToND.Lock_x = LOCK_UNDEF;	/* Init Lamport locks */
	MSG_QUEUES(sp)->ToND.Lock_y = LOCK_UNDEF;
	MSG_QUEUES(sp)->FromND.Lock_x = LOCK_UNDEF;
	MSG_QUEUES(sp)->FromND.Lock_y = LOCK_UNDEF;
	MSG_QUEUES(sp)->ReplyND.Lock_x = LOCK_UNDEF;
	MSG_QUEUES(sp)->ReplyND.Lock_y = LOCK_UNDEF;
	MSG_QUEUES(sp)->Flags = MSGFLAG_MSG_IN_EMPTY;	/* Plenty of room now... */
	splx(s);

	/* If the to-ND server thread was paused waiting for room, wake it up now. */
	if ( (sp->flags & ND_SERVER_THREAD_PAUSED) != 0 )
		thread_wakeup( (void *)&sp->server_thread );
}

 void
ND_update_message_system( ND_var_t *sp )
{
	if ( ! sp->present )
		return;

	MSG_QUEUES(sp)->kernel_port = sp->kernel_port;	/* Set port info up. */
	MSG_QUEUES(sp)->debug_port = sp->debug_port;
	MSG_QUEUES(sp)->service_port = sp->service_port;
	MSG_QUEUES(sp)->service_replyport = sp->service_replyport;
}


/* Generic 'how to write to a circular buffer' code, also called from ND_reply().
 * Try to inline it here, where it gets used most frequently. 
 */
 void inline
ND_writequeue( char *buf, NDQueue *q, char *src, int size )
{
	int cnt;
	
	if ( (cnt = MSGBUF_SIZE - q->Head) < size )	/* Do copy in two pieces */
	{
	    bcopy( src, &buf[q->Head], cnt );
	    src += cnt;				/* Bump src to next byte */
	    size -= cnt;
	    
	    bcopy( src, buf, size );		/* Copy 2nd half */
	    q->Head = size;			/* Wrap head back around */
	}
	else					/* Do copy in one piece */
	{
	    bcopy( src, &buf[q->Head], size );	/* Copy off queue */
	    q->Head += size;			/* Bump up head pointer */
	}
}

 
/*
 * Forward a message to the ND board for processing.
 */
 void
ND_msg_server_loop ( void )
{
	msg_header_t *msg;
	NDQueue *q;
	int size;
	int i;
	int s;	/* Saved IPL */
	ND_var_t * sp = (ND_var_t *) 0;
	kern_return_t r;

	for (i = 0; i < SLOTCOUNT; i++)
	{
		if ( ND_var[i].server_thread == current_thread() )
		{
			sp = &ND_var[i];
			break;
		}
	}
	if ( sp == (ND_var_t *)0 )
	{
		printf( "ND_msg_server_loop: couldn't find my slot!!!\n" );
		thread_terminate( current_thread() );
		thread_halt_self();
	}

	msg = (msg_header_t *) kalloc(MSG_SIZE_MAX);

	sp->flags |= ND_SERVER_THREAD_RUNNING;
	thread_wakeup( (void *)&sp->flags );	/* Let the kernel continue */
	
	/* Activate transparent translation for this service thread. */
	pmap_tt(current_thread(), 1, sp->board_addr, ND_BOARD_SIZE, 0);
	q = &MSG_QUEUES(sp)->ToND;		/* Message queue from host to ND */
	
	/*
	 * FOREVER:
	 *	Wait for a message.
	 *	Transfer any messages from the Mach message system  to the
	 *	ND reply queue for forwarding to their ultimate destination.
	 */

	while ( (sp->flags & ND_STOP_SERVER_THREAD) == 0 )
	{
	    msg->msg_local_port = sp->ND_portset;
	    msg->msg_size = MSG_SIZE_MAX;
	    r = msg_receive( msg, RCV_INTERRUPT, 0 );
	    switch ( r )
	    {
		    case RCV_SUCCESS:
			    break;
		    case RCV_INTERRUPTED: /* Got a soft interrupt. Thread dying? */
			    if (thread_should_halt(current_thread()))
			    {	    /* Supposed to halt? Do it cleanly! */
				    simple_lock( &sp->flags_lock );
				    sp->flags |= ND_STOP_SERVER_THREAD;
				    simple_unlock( &sp->flags_lock );
				    break;
			    }
			    continue;
		    case RCV_INVALID_PORT:     /* Port destroyed, shutting down. */
			    break;
		    default:
			    printf( "ND_msg_server_loop: weird error (%d)\n", r );
			    continue;
	    }
	    /* We either got a message or are shutting down.  What'll it be? */
	    if( (sp->flags & ND_STOP_SERVER_THREAD) != 0)
	    	break;	/* Go halt this thread. */

	    /* Forward the message to the ND board, after doing out-of-line
	     * data conversions and such.
	     */
	    if ( msg->msg_simple != TRUE && valid_msg( sp, msg ) == FALSE )
	    {
		    continue;	/* Hey, what can we do??? */
	    }
    
	    q = &(MSG_QUEUES(sp)->ToND);	/* Addr of queue control struct */
	    size = (msg->msg_size + 3) & ~3;	/* Size rounded up to int align */

	    /* Wait for enough space to enqueue the message to appear */
	    while (   size >= (MSGBUF_SIZE - BYTES_ON_QUEUE(q))
	    	   && (sp->flags & ND_STOP_SERVER_THREAD) == 0 )
	    {
		/* Flag board to send an interrupt when space is freed in buffer */
		s = spl_ND();	/* Block ND interrupts to avoid race condition */
		simple_lock( &sp->flags_lock );
		MSG_QUEUES(sp)->Flags |= (MSGFLAG_MSG_IN_EMPTY | MSGFLAG_MSG_IN_LOW);
	        sp->flags |= ND_SERVER_THREAD_PAUSED;
		simple_unlock( &sp->flags_lock );
		if ( (sp->flags & ND_INTR_PENDING) == 0 )
		{	/* Wait for intr. */
		    assert_wait( (void *)&sp->server_thread, FALSE );
		    thread_block();
		}
		simple_lock( &sp->flags_lock );
	        sp->flags &= ~(ND_SERVER_THREAD_PAUSED | ND_INTR_PENDING);
		simple_unlock( &sp->flags_lock );
		splx( s );	/* Permit ND interrupts */
	    }
	    
	    /* We could also have been woken up to be shut down. */
	    if( (sp->flags & ND_STOP_SERVER_THREAD) != 0)
	    	break;

	    simple_lock( &sp->flags_lock );
	    MSG_QUEUES(sp)->Flags &= ~(MSGFLAG_MSG_IN_EMPTY | MSGFLAG_MSG_IN_LOW);
	    simple_unlock( &sp->flags_lock );

	    ND_LamportLock( sp, q );
	    ND_writequeue( (char *)MSGBUF_HOST_TO_860(sp), q, (char *)msg, size );
	    ND_LamportUnlock( q );	/* It's a macro... */
	    ND_INT860(sp);		/* Let it know that it has a message */
	}
	/*
	 * time to die. Notify main thread that we're dead.
	 */
	/* Shut down transparent translation for this service thread. */
	pmap_tt(current_thread(), 0, 0, 0, 0);
	kfree( (void *) msg, MSG_SIZE_MAX );
	simple_lock( &sp->flags_lock );
	sp->flags &= ~ND_SERVER_THREAD_RUNNING;
	simple_unlock( &sp->flags_lock );
	thread_wakeup(&sp->flags);
	thread_terminate( current_thread() );
	thread_halt_self();
	/*NOTREACHED*/
}


/*
 * Make sure that this message is in a form we can support, and take care
 * of out of line data.  We can assume that the message structure is sane
 * at this point, so we don't look for bad lengths and whatnot.
 *
 * Out of line data is moved into our paging object, a threadless task.
 */
 static boolean_t
valid_msg( ND_var_t *sp, msg_header_t *msg )
{
	caddr_t saddr;
	caddr_t staddr;
	caddr_t endaddr;
	msg_type_long_t *tp;
	unsigned int tn;
	unsigned long ts;
	register long elts;
	vm_size_t numbytes;
	
	if ( msg->msg_simple == TRUE )	/* Safety check */
		return TRUE;
		
	saddr = (caddr_t)(msg + 1);	/* Start after header */
	endaddr = ((caddr_t)msg) + msg->msg_size;	/* And stop at the end... */
	
	while ( saddr < endaddr )
	{
		tp = (msg_type_long_t *)saddr;
		staddr = saddr;
		if (tp->msg_type_header.msg_type_longform) {
			tn = tp->msg_type_long_name;
			ts = tp->msg_type_long_size;
			elts = tp->msg_type_long_number;
			saddr += sizeof(msg_type_long_t);
		} else {
			tn = tp->msg_type_header.msg_type_name;
			ts = tp->msg_type_header.msg_type_size;
			elts = tp->msg_type_header.msg_type_number;
			saddr += sizeof(msg_type_t);
		}

		numbytes = ((elts * ts) + 7) >> 3;
		
		/*
		 * At this point, saddr points to the typed data, 
		 * and staddr points to the type info for the data.
		 * numbytes holds the lenght of the typed data in bytes.
		 *
		 * Next, we evaluate the type to see if the data is out of line.
		 * If so, we transfer the data from out task to the paging 'task',
		 * a task with no threads which acts as a backing store for the actual
		 * task running on the ND board.
		 * We also fix up the data pointer to reflect the new address.
		 */
		if ( tp->msg_type_header.msg_type_inline == FALSE )
		{
			vm_offset_t	src_addr;
			vm_offset_t	dest_addr;
			kern_return_t	r;
			
			src_addr = *((vm_offset_t *)saddr);
			r = ND_vm_move(	sp->map_self,	/* Our task vm_map_t */
					src_addr,	/* Addr of data in our task */
					sp->map_pager,	/* Paging 'task' vm_map_t */
					numbytes,	/* Bytes to move */
					TRUE,		/* Deallocate from our task */
					&dest_addr );	/* Fill in new task addr */
			if ( r != KERN_SUCCESS )
			{
				printf( "NextDimension: vm_move returned %d\n", r );
				return FALSE;
			}
			*((vm_offset_t *)saddr) = dest_addr;	/* ND task's address */
			saddr += sizeof (caddr_t);	/* Skip over pointer to data */ 
		}
		else
			saddr += ((numbytes+3) & (~0x3));/* Skip over the typed data */
	}
	return TRUE;
}

/*
 *	Process a message from the ND board to the host containing out of band data.
 *	Out of band data is copied from the paging task to the server task.
 *	The copied data is marked as msg_type_deallocate, to get it freed from
 *	our task space as it propagates through the message system.
 */
 boolean_t
ND_oob_msg_to_host( ND_var_t *sp, msg_header_t *msg )
{
	caddr_t saddr;
	caddr_t staddr;
	caddr_t endaddr;
	msg_type_long_t *tp;
	unsigned int tn;
	unsigned long ts;
	register long elts;
	vm_size_t numbytes;
	
	if ( msg->msg_simple == TRUE )	/* Safety check */
		return TRUE;
		
	saddr = (caddr_t)(msg + 1);	/* Start after header */
	endaddr = ((caddr_t)msg) + msg->msg_size;	/* And stop at the end... */
	
	while ( saddr < endaddr )
	{
		tp = (msg_type_long_t *)saddr;
		staddr = saddr;
		if (tp->msg_type_header.msg_type_longform) {
			tn = tp->msg_type_long_name;
			ts = tp->msg_type_long_size;
			elts = tp->msg_type_long_number;
			saddr += sizeof(msg_type_long_t);
		} else {
			tn = tp->msg_type_header.msg_type_name;
			ts = tp->msg_type_header.msg_type_size;
			elts = tp->msg_type_header.msg_type_number;
			saddr += sizeof(msg_type_t);
		}

		numbytes = ((elts * ts) + 7) >> 3;
		
		/*
		 * At this point, saddr points to the typed data, 
		 * and staddr points to the type info for the data.
		 * numbytes holds the lenght of the typed data in bytes.
		 *
		 * Next, we evaluate the type to see if the data is out of line.
		 * If so, we transfer the data from paging task to our task.
		 * We also fix up the data pointer to reflect the new address,
		 * and mark the type field to deallocate VM from our task.
		 */
		if ( tp->msg_type_header.msg_type_inline == FALSE )
		{
			vm_offset_t	src_addr;
			vm_offset_t	dest_addr;
			boolean_t	dealloc;
			kern_return_t	r;
			
			src_addr = *((vm_offset_t *)saddr);
			dealloc = tp->msg_type_header.msg_type_deallocate;
			r = ND_vm_move(	sp->map_pager,	/* Our pager vm_map_t */
					src_addr,	/* Addr of data in pager */
					sp->map_self,	/* Our task vm_map_t */
					numbytes,	/* Bytes to move */
					dealloc,	/* deallocate from pager? */
					&dest_addr );	/* Fill in new task addr */
			if ( r != KERN_SUCCESS )
			{
				printf( "NextDimension: vm_move returned %d\n", r );
				return FALSE;
			}
			tp->msg_type_header.msg_type_deallocate = TRUE;
			*((vm_offset_t *)saddr) = dest_addr;  /* Our task's address */
			saddr += sizeof (caddr_t);  /* Skip over pointer to data */ 
		}
		else
			saddr += ((numbytes+3) & (~0x3));/* Skip over typed data */
	}
	return TRUE;
}


