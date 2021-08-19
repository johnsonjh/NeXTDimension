/*
 * Copyright 1989 NeXT, Inc.
 *
 * NextDimension prototype loadable kernel server.
 *
 * This module contains the code which sends a message out the reply port
 * on receipt of an interrupt from the NextDimension board.
 */
#include "ND_patches.h"			/* Must be FIRST! */
#include <machine/spl.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/message.h>
#include <sys/msg_type.h>
#include <sys/mig_errors.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/kernel.h>
#include <nextdev/slot.h>
#include <next/pcb.h>
#include <next/scr.h>
#include "ND_var.h"
#include <ND/NDmsg.h>
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
void ND_writequeue( char *, NDQueue *, char *, int );


//#define DEBUG_TRIGGERS

extern ND_var_t ND_var[];

static void sendServiceReply( ND_var_t *, msg_header_t * );
static void readqueue( char *, NDQueue *, char *, int );
static void snoopqueue( char *, NDQueue *, char *, int );

union Reply
{
	msg_header_t Head;
	char data[MSG_SIZE_MAX];
};

typedef struct {
	msg_header_t Head;
	msg_type_t RetCodeType;
	kern_return_t RetCode;
} ServiceReply;


/*
 * ND_reply_server_loop:
 *
 *	This routine sleeps, waiting for interrupts from the ND board.  On 
 *	receipt of an interrupt, it reads messages from the queue, forwarding 
 *	the messages to the Mach message system for routing to their final
 *	destination.
 */
 void
ND_reply_server_loop( void )
{
	ND_var_t * sp = (ND_var_t *) 0;
	ND_conio_t *msg;
	union Reply *Reply;
	int ReplySize	= ND_MAX_REPLYSIZE;	// Set starting size for Reply buffer
	ServiceReply *ServReply;
	kern_return_t r;
	NDQueue *q;
	int send_err_reported = 0;
	int type;
	int i;
	int s;
	
	for (i = 0; i < SLOTCOUNT; i++)
	{
		if ( ND_var[i].reply_thread == current_thread() )
		{
			sp = &ND_var[i];
			break;
		}
	}
	if ( sp == (ND_var_t *)0 )
	{
		printf( "ND_reply_server_loop: couldn't find my slot!!!\n" );
		thread_terminate( current_thread() );
		thread_halt_self();
	}
	
	/* Allocate storage for biggest possible message and pun it over our types */
	Reply = (union Reply *) kalloc( ReplySize );
	msg = (ND_conio_t *) Reply;
	ServReply = (ServiceReply *) kalloc( ND_MAX_REPLYSIZE );

	/* Set up initial conditions with the ND board. */
	ND_CONIO_MSG_ID( sp ) = ND_CONIO_IDLE;
	q = &MSG_QUEUES(sp)->FromND;		/* Message queue from ND to host */
	MSG_QUEUES(sp)->Flags |= MSGFLAG_MSG_OUT_AVAIL;	/* Please send intr. */
	
	sp->flags |= ND_REPLY_THREAD_RUNNING;
	thread_wakeup( (void *)&sp->flags );	/* Let the kernel continue */
		

	/* Activate transparent translation for this service thread. */
	pmap_tt(current_thread(), 1, sp->board_addr, ND_BOARD_SIZE, 0);

	/*
	 * FOREVER:
	 *	Wait for an interrupt.
	 *	Forward any console I/O messages.
	 *	Transfer any messages from the ND reply queue to the
	 *	Mach message system for forwarding to their ultimate destination.
	 */
	while ( 1 )
	{
	    if( (sp->flags & ND_STOP_REPLY_THREAD) == 0)
	    {
		s = spl_ND();
		simple_lock(&sp->flags_lock);
	        sp->flags |= ND_REPLY_THREAD_PAUSED;
		simple_unlock(&sp->flags_lock);
		if ( (sp->flags & ND_INTR_PENDING) == 0 )
		{   /* Wait for intr. */
		    assert_wait( (void *)&sp->reply_thread, FALSE );
		    thread_block();
		}
		simple_lock(&sp->flags_lock);
	        sp->flags &= ~(ND_REPLY_THREAD_PAUSED | ND_INTR_PENDING);
		simple_unlock(&sp->flags_lock);
		splx(s);
	    }
	    if(sp->flags & ND_STOP_REPLY_THREAD)
	    {
		/*
		 * time to die. Notify main thread that we're dead.
		 */
		/* Shut down transparent translation for this service thread. */
		pmap_tt(current_thread(), 0, 0, 0, 0);
		kfree( (void *) Reply, ReplySize );
		kfree( (void *) ServReply, ND_MAX_REPLYSIZE );
		simple_lock(&sp->flags_lock);
		sp->flags &= ~ND_REPLY_THREAD_RUNNING;
		simple_unlock(&sp->flags_lock);
		thread_wakeup(&sp->flags);
		thread_terminate( current_thread() );
		thread_halt_self();
		/*NOTREACHED*/
	    }
		
	    type = ND_CONIO_MSG_ID( sp );
	    switch ( type )
	    {
		case ND_CONIO_MESSAGE_AVAIL:
		    break;
		case ND_CONIO_OUT:
		case ND_CONIO_IN_CHAR:
		case ND_CONIO_IN_STRING:
		case ND_CONIO_EXIT:
		    /*
		     * Send a message to the debug_port indicating that
		     * an interrupt occured, containing the console I/O
		     * information.
		     */
		    if ( sp->debug_port == PORT_NULL )
		    {
		    	/* Fake a send ack to board. */
		    	send_err_reported = 0; /* Clear when debug_port gone */
			ND_CONIO_IN_COUNT( sp ) = 0;
			*((int *)ND_CONIO_OUT_BUF(sp)) = 0;
			ND_CONIO_MSG_ID( sp ) = ND_CONIO_IDLE;
		    	break;	/* Nobody is interested in these messages */
		    }
		    msg->Head.msg_simple = TRUE;
		    msg->Head.msg_size = sizeof (ND_conio_t);
		    msg->Head.msg_type = MSG_TYPE_NORMAL;
		    msg->Head.msg_local_port = PORT_NULL;
		    msg->Head.msg_remote_port = sp->debug_port;
		    msg->Slot = (sp - ND_var) << 1; /* Hide the slot number as ID */
		    
		    msg->Head.msg_id = type;
		    if ( (msg->Length = ND_CONIO_OUT_COUNT( sp )) != 0 )
			bcopy(ND_CONIO_OUT_BUF(sp), msg->Data, (msg->Length + 3) & ~3);
		    else
			msg->Data[0] = 0;
		    /* Finally, send the message */
		    r = msg_send((msg_header_t *)&msg->Head, MSG_OPTION_NONE, 0);
		    
		    /* Post a send ack to board. */
		    if ( type == ND_CONIO_IN_CHAR || type == ND_CONIO_IN_STRING )
			ND_CONIO_MSG_ID( sp ) = ND_CONIO_IN_STAND_BY;
		    else
			ND_CONIO_MSG_ID( sp ) = ND_CONIO_IDLE;

		    if (r != KERN_SUCCESS)
		    {
		    	if ( ! send_err_reported++ )
			{
			    printf("NextDimension: could not deliver msg.");
			    printf("(error %d)\n",r );
			}
		    }
		    else
		    	send_err_reported = 0;
		    break;
	    }
	    while ( BYTES_ON_QUEUE(q) >= sizeof(msg_header_t) )
	    {
		/*
		 * A message is on the queue to be sent to the host.
		 * Fetch the message into Reply, and do a msg_send().
		 *
		 * We do some error checking in case we got a message
		 * interrupt before the full message was enqueued.
		 * If this happens, it is either a software bug on the ND side
		 * or a hardware glitch.
		 */
		msg_header_t hdr;

		snoopqueue( MSGBUF_860_TO_HOST(sp), q, (char *)&hdr, sizeof hdr );
		if ( BYTES_ON_QUEUE(q) < hdr.msg_size )
		{
			printf( "On queue %d, need hdr %d; head 0x%x, tail 0x%x\n",
				BYTES_ON_QUEUE(q),
				hdr.msg_size,
				q->Head, q->Tail );
			break;
		}
		/* If the message won't fit in the Reply buffer, grow the buffer */
		if ( hdr.msg_size > ReplySize )
		{
			kfree( (void *)Reply, ReplySize );
			ReplySize = hdr.msg_size;
			Reply = (union Reply *) kalloc( ReplySize );
		}
		readqueue(MSGBUF_860_TO_HOST(sp),q,(char *)&Reply->data,hdr.msg_size);
		/* Is this a request for kernel services? */
		if ( Reply->Head.msg_remote_port == sp->service_port )
		{
			port_t	save_local_port = Reply->Head.msg_local_port;
			
			Reply->Head.msg_local_port = (port_t)sp; /* Yes, a hack... */
			ND_Kern_server( &Reply->Head, &ServReply->Head );
			if (ServReply->RetCode != MIG_NO_REPLY)
			{
			    if ( ServReply->Head.msg_size > ND_MAX_REPLYSIZE )
				printf("NextDimension: Msg ID %d reply too large!\n",
						(ServReply->Head.msg_id - 100) );
			    ServReply->Head.msg_local_port = save_local_port;
			    sendServiceReply( sp, &ServReply->Head );
			}
		}
		else
		{
		    /* Grind any out of band data in the message. */
		    if ( Reply->Head.msg_simple != TRUE )
			ND_oob_msg_to_host( sp, (msg_header_t *)&Reply->Head );
		    /* Send the message */
		    r = msg_send((msg_header_t *)&Reply->Head, MSG_OPTION_NONE, 0);
		    if (r != KERN_SUCCESS)
		    {
		    	if ( ! send_err_reported++ )
			{
			    printf("NextDimension: could not deliver msg.");
			    printf("(error %d)\n",r );
			}
		    }
		    else
		    	send_err_reported = 0;
		    break;
		}
	    }
	}
	/* NOT REACHED */
}

/*
 * Send a message to the kernel services reply queue.
 */
 static void
sendServiceReply( ND_var_t * sp , msg_header_t * msg )
{
	NDQueue *q;
	int size;
	int i;
	int s;	/* Saved IPL */
	kern_return_t r;

	q = &(MSG_QUEUES(sp)->ReplyND);	/* Addr of queue control struct */
	size = (msg->msg_size + 3) & ~3;	/* Size rounded up to int align */
	
	/* Wait for enough space to enqueue the message to appear */
	while (   size >= (MSGBUF_SIZE - BYTES_ON_QUEUE(q))
		&& (sp->flags & ND_STOP_REPLY_THREAD) == 0 )
	{
	    /* Flag board to send an interrupt when space is freed in buffer */
	    s = spl_ND();	/* Block ND interrupts to avoid race condition */
	    simple_lock( &sp->flags_lock );
	    MSG_QUEUES(sp)->Flags |= (MSGFLAG_MSG_REPLY_EMPTY | MSGFLAG_MSG_REPLY_LOW);
	    sp->flags |= ND_REPLY_THREAD_PAUSED;
	    simple_unlock( &sp->flags_lock );
	    if ( (sp->flags & ND_INTR_PENDING) == 0 )
	    {	/* Wait for intr. */
		assert_wait( (void *)&sp->server_thread, FALSE );
		thread_block();
	    }
	    simple_lock( &sp->flags_lock );
	    sp->flags &= ~(ND_REPLY_THREAD_PAUSED | ND_INTR_PENDING);
	    simple_unlock( &sp->flags_lock );
	    splx( s );	/* Permit ND interrupts */
	}
	
	/* We could also have been woken up to be shut down. */
	if( (sp->flags & ND_STOP_REPLY_THREAD) != 0)
	    return;
	
	simple_lock( &sp->flags_lock );
	MSG_QUEUES(sp)->Flags &= ~(MSGFLAG_MSG_REPLY_EMPTY | MSGFLAG_MSG_REPLY_LOW);
	simple_unlock( &sp->flags_lock );
	
	ND_LamportLock( sp, q );
	ND_writequeue( (char *)MSGBUF_REPLY_TO_860(sp), q, (char *)msg, size );
	ND_LamportUnlock( q );	/* It's a macro... */
	ND_INT860(sp);		/* Let it know that it has a message */
}

/*
 * Read data off of our circular buffer.
 */
 static void
readqueue( char * buf, NDQueue *q, char *dst, int size )
{
	int cnt;
	
	if ( size <= 0 )
		return;
	if ( (cnt = MSGBUF_SIZE - q->Tail) < size )	/* Do copy in two pieces */
	{
		bcopy( &buf[q->Tail], dst, cnt );
		dst += cnt;				/* Bump dst to next byte */
		size -= cnt;
		
		bcopy( buf, dst, size );		/* Copy 2nd half */
		q->Tail = size;				/* Wrap tail back around */
	}
	else						/* Do copy in one piece */
	{
		bcopy( &buf[q->Tail], dst, size );	/* Copy off queue */
		q->Tail += size;			/* Bump up tail pointer */
	}
}

/*
 * Fetch data from the circular buffer, but don't move the tail pointer.
 */
 static void
snoopqueue( char * buf, NDQueue *q, char *dst, int size )
{
	int cnt;
	
	if ( size <= 0 )
		return;
	if ( (cnt = MSGBUF_SIZE - q->Tail) < size )	/* Do copy in two pieces */
	{
		bcopy( &buf[q->Tail], dst, cnt );
		dst += cnt;				/* Bump dst to next byte */
		size -= cnt;
		
		bcopy( buf, dst, size );		/* Copy 2nd half */
	}
	else						/* Do copy in one piece */
	{
		bcopy( &buf[q->Tail], dst, size );	/* Copy off queue */
	}
}








