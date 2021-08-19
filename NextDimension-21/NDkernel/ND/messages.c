/*
 * messages.c
 *
 *	This module handles the message queues with the host and provides Mach message
 *	emulation services for the rest of the kernel and applications.  Just as in
 *	Mach, message delivery is guaranteed to be sequential, although it may not
 *	be obvious from the code.
 */
#include "i860/proc.h"
#include "ND/NDmsg.h"
#include "ND/NDreg.h"
#include <i860/vm_param.h>
#include <sys/message.h>

//#define DEBUG_TRIGGERS

/*
 *	Queues, as filched from /usr/include/chthreads.h.  These suited my
 *	needs more than the stuff in sys/queues.h
 */
typedef struct queue {
	struct queue_item *head;
	struct queue_item *tail;
} *queue_t;

typedef struct queue_item {
	struct queue_item *next;
} *queue_item_t;

#define	NO_QUEUE_ITEM	((queue_item_t) 0)

#define	QUEUE_INITIALIZER	{ NO_QUEUE_ITEM, NO_QUEUE_ITEM }

#define	queue_alloc()	((queue_t) kcalloc(1, sizeof(struct queue)))
#define	queue_init(q)	((q)->head = (q)->tail = 0)
#define	queue_free(q)	kfree((char *) (q))

#define	queue_enq(q, x) { \
	(x)->next = 0; \
	if ((q)->tail == 0) \
		(q)->head = (queue_item_t) (x); \
	else \
		(q)->tail->next = (queue_item_t) (x); \
	(q)->tail = (queue_item_t) (x); \
}

#define	queue_preq(q, x) { \
	if ((q)->tail == 0) \
		(q)->tail = (queue_item_t) (x); \
	((queue_item_t) (x))->next = (q)->head; \
	(q)->head = (queue_item_t) (x); \
}

#define	queue_head(q, t)	((t) ((q)->head))

#define	queue_deq(q, t, x)	if (((x) = (t) ((q)->head)) != 0 && \
				    ((q)->head = (queue_item_t) ((x)->next)) == 0) \
					(q)->tail = 0; \
				else

#define	queue_map(q, t, f) { \
	register queue_item_t x, next; \
	for (x = (queue_item_t) ((q)->head); x != 0; x = next) { \
		next = x->next; \
		(*(f))((t) x); \
	} \
}


typedef struct _msg_tab
{
	struct _msg_tab	*next;	/* Link to next item on queue. */
	int		flags;
	msg_return_t	status;		/* 0 on success, error code otherwise */
	msg_header_t	*data; 
} msg_tab;

#define MSG_TAB_WAITING		1		/* Waiting for data to be loaded */
#define MSG_TAB_LOADED		2		/* Data has been filled in. */
#define MSG_TAB_ERROR		4
#define MSG_TAB_PORT_DEATH	8		/* Port has died. */

typedef struct _PortList
{
	struct	_PortList *next;
	port_t	port;
	struct queue	unclaimed_q;
	struct queue	pending_q;
} PortList;

static PortList *PPortList = (PortList *) 0;

void AddPortToList( port_t );
void DeletePortFromList( port_t );

static void readqueue( char *, NDQueue *, char *, int );
static void snoopqueue( char *, NDQueue *, char *, int );
static void skipqueue( NDQueue *, int );
static msg_return_t writequeue( char *, NDQueue *, char *, int );

extern void *kmalloc( unsigned );
extern void *kcalloc( unsigned, unsigned );
extern void kfree( void *);

/*
 * InitMessages()
 *
 *	Initialize the message system.  This is done here instead of in Messages,
 *	so we can get the host interface ready to run very early in the boot process.
 */
 void
InitMessages()
{
	/* Initialize the message queues */
	AddPortToList( MSG_QUEUES->kernel_port );
	MSG_QUEUES->Flags |= MSGFLAG_MSG_SYS_READY;	
}

/*
 * Messages:
 *
 *	Messages() sleeps on the host->ND message buffer, and is awakened by
 *	the hardware interrupt handler.  When data becomes available, the task
 *	wakes up and reads a Mach message header off of the queue.  The destination
 *	port is hashed and used as a key to enter the message into a linked list to
 *	wait for a reader, or to find the waiting reader.
 *
 *	Messages() runs as a lightweight process in the kernel address space.
 */
Messages()
{
	msg_header_t hdr;
	queue_t unclaimed_q;
	queue_t pending_q;
	msg_tab *head;
	msg_tab *result;
	register int size;
	PortList *p;
	int s;
	
	while( 1 )
	{
	    while( BYTES_ON_QUEUE(&MSG_QUEUES->ToND) >= sizeof hdr )
	    {
		readqueue( MSGBUF_HOST_TO_860, &MSG_QUEUES->ToND,
				(char *)&hdr, sizeof hdr );
		if ( (size = hdr.msg_size - sizeof hdr) > 0 )
		{
		    s = splhigh();
		    while (BYTES_ON_QUEUE(&MSG_QUEUES->ToND) < size)
		    {
			Sleep( &MSG_QUEUES->ToND, PIDLE ); /* Wait for rest of msg. */
		    }
		    splx(s);
		}
#if 0
printf( "msg_simple %D, msg_size %D, msg_type %D, ", hdr.msg_simple, hdr.msg_size, hdr.msg_type );
printf( "msg_local_port %D, msg_remote_port %D, msg_id %D\n", hdr.msg_local_port, hdr.msg_remote_port, hdr.msg_id );
printf( " queue head %d, tail %d\n", MSG_QUEUES->ToND.Head, MSG_QUEUES->ToND.Tail);
#endif
		/* Select the queue set based on the message destination port */
		p = PPortList;
		while ( p != (PortList *) 0 )
		{
			if ( p->port == hdr.msg_local_port )
			{
				unclaimed_q = &p->unclaimed_q;
				pending_q = &p->pending_q;
				break;
			}
			p = p->next;
		}
		if ( p == (PortList *) 0 )    /* Not a registered port. Discard msg */
		{
		    if ( size > 0 )
			skipqueue( &MSG_QUEUES->ToND, size );
		    continue;
		}
		/* Is there a pending reader for this message? */
		queue_deq(pending_q, msg_tab *, result);
		if ( result != (msg_tab *)0 )	/* Have a pending read. */
		{
		    /* Copy the header */
		    bcopy( &hdr, result->data, sizeof hdr );
		    if ( result->data->msg_size <= hdr.msg_size ) /* Will it fit? */
		    {
			/* Copy the remaining buffer of data */
			if ( size > 0 )
			    readqueue( MSGBUF_HOST_TO_860, &MSG_QUEUES->ToND,
				    (char *)&result->data[1], size );
			result->flags = MSG_TAB_LOADED;
			result->status = RCV_SUCCESS;
			
			/* Pull the result out of the queue */
			Wakeup( result );	/* Wake up the reader. */
			continue;	/* On to the next message */
		    }
		    else    /* Message didn't fit. Send an error. Msg is unclaimed */
		    {
			result->flags = MSG_TAB_ERROR;
			result->status = RCV_TOO_LARGE;
			Wakeup( result );	/* Wake up the reader. */
			/* Fall thru to place message on Unclaimed queue for now */
		    }
		}
		/* Tail insert the message on the Unclaimed queue for now. */
		
		result = (msg_tab *) kmalloc( sizeof (msg_tab) );
		if ( result == (msg_tab *)0 )	/* Can't happen.... */
			panic( "Messages: out of memory" );
		result->data = (msg_header_t *) kmalloc( hdr.msg_size );
		if ( result->data == (msg_header_t *)0 )
			panic( "Messages: out of memory" );
			
		/* Copy out the message header */
		bcopy( &hdr, result->data, sizeof hdr );
		/* Copy the remaining buffer of data */
		if ( size > 0 )
		    readqueue( MSGBUF_HOST_TO_860, &MSG_QUEUES->ToND,
			    (char *)&result->data[1], size );
			
		result->flags = MSG_TAB_LOADED;
		result->status = RCV_SUCCESS;
		queue_enq(unclaimed_q, result)
		if ( (MSG_QUEUES->Flags & MSGFLAG_MSG_IN_LOW) &&
			BYTES_ON_QUEUE(&MSG_QUEUES->ToND) < (MSGBUF_SIZE/2) )
		{
			ping_driver();
		}
		/* Continue with the next message */
	    }
	    /* Let the driver know that we have room now. */
	    if ( (MSG_QUEUES->Flags & MSGFLAG_MSG_IN_EMPTY) != 0 )
		    ping_driver();
	    /*
	     * Wait for more data to process.
	     * The wakeup is driven by hardware interrupts, hence the spl() hackery.
	     */
	    s = splhigh();
	    if ( BYTES_ON_QUEUE(&MSG_QUEUES->ToND) < sizeof hdr )
	    	Sleep( &MSG_QUEUES->ToND, PIDLE );
	    splx(s);
	}
}

/*
 * A non-mallocing receive on our very own private message buffer just 
 * for things like pager replies.  This special queue is used for service_replyport
 * messages ONLY.
 */
 static msg_return_t
ServiceMsg_receive( msg_header_t *header, msg_option_t option, msg_timeout_t timeout )
{
	extern struct proc *P;	/* The global current process */
	msg_header_t hdr;
	register int s;
	register int priority = P->p_pri;
	
	/*
	 * Wait for data to process.
	 * The wakeup is driven by hardware interrupts, hence the spl() hackery.
	 */
	s = splhigh();
	while( BYTES_ON_QUEUE(&MSG_QUEUES->ReplyND) < sizeof hdr )
	    Sleep( &MSG_QUEUES->ReplyND, priority );
	splx(s);
	snoopqueue( MSGBUF_REPLY_TO_860, &MSG_QUEUES->ReplyND,
			(char *)&hdr, sizeof hdr );

	/* If the message won't fit, throw it back. */
	if ( hdr.msg_size > header->msg_size )
	{
		bcopy( (char *)&hdr, (char *)header, sizeof (msg_header_t) );
		return RCV_TOO_LARGE;
	}

	s = splhigh();
	while (BYTES_ON_QUEUE(&MSG_QUEUES->ReplyND) < hdr.msg_size)
	{
	    Sleep( &MSG_QUEUES->ReplyND, priority ); /* Wait for rest of msg. */
	}
	splx(s);
	
	readqueue( MSGBUF_REPLY_TO_860, &MSG_QUEUES->ReplyND,
			(char *)header, hdr.msg_size );
	/* Wake up any possible sleepers on the host side. */		
	if ( (MSG_QUEUES->Flags & MSGFLAG_MSG_REPLY_LOW) &&
		BYTES_ON_QUEUE(&MSG_QUEUES->ReplyND) < (MSGBUF_SIZE/2) )
	{
		ping_driver();
	}
	else if ( (MSG_QUEUES->Flags & MSGFLAG_MSG_REPLY_EMPTY) &&
		BYTES_ON_QUEUE(&MSG_QUEUES->ReplyND) == 0 )
	{
		ping_driver();
	}

	return RCV_SUCCESS;
}

/*
 * Internal kernel routines used to set up or tear down message queues
 * for ports.  Once the port allocate/deallocate stuff is stable, I can
 * tighten these up.  Probably not really a performance hit, though.
 */
 void
AddPortToList( port_t port )
{
	PortList *p;
	PortList *new;
	if ((new = (PortList *)kcalloc(1, sizeof (PortList))) == (PortList*)0)
		panic("AddPortToList: out of memory." );
	new->port = port;
	
	new->next = PPortList;
	PPortList = new;
}

 void
DeletePortFromList( port_t port )
{
	extern struct proc *P;	/* The global current process */
	register int priority = P->p_pri;
	PortList *p;
	PortList *prev;
	msg_tab * result;

	if ( PPortList == (PortList *) 0 )
		return;
	p = PPortList;
	prev = (PortList *) 0;
	while ( p != (PortList *) 0 )
	{
		if ( p->port == port )	/* found it */
			break;
		prev = p;
		p = p->next;
	}
	if ( p == (PortList *) 0 )
		return;		/* Bogus port.... */
	if ( p != PPortList )
		prev->next = p->next;
	else
		PPortList = p->next;

	/* Wake anyone waiting for a message on this port.*/
	while ( queue_head(&p->pending_q, queue_item_t) != NO_QUEUE_ITEM )
	{
		queue_deq(&p->pending_q, msg_tab *, result);
		result->flags = MSG_TAB_PORT_DEATH;
		result->status = RCV_INVALID_PORT;
		Wakeup( result );
	}
	/* Reclaim storage from unclaimed messages */
	while ( queue_head(&p->unclaimed_q, queue_item_t) != NO_QUEUE_ITEM )
	{
		queue_deq(&p->pending_q, msg_tab *, result);
		kfree( result->data );
		kfree( result );
	}

	kfree( p );
}

/*
 * The official Mach message functions.
 */

/*
 * msg_send:
 *
 *	Send a message as specified in the header.
 *	For the moment, we ignore the options and timeout, as we have no
 *	timer capability.  Every msg_send is a blocking send.
 */
 msg_return_t
msg_send( msg_header_t *header, msg_option_t option, msg_timeout_t timeout )
{
	msg_return_t status;
	int s;
	extern struct proc *P;	/* The global current process */
	register int priority = P->p_pri;

	if ( header->msg_size >= MSGBUF_SIZE )
		return( SEND_MSG_TOO_LARGE );

	if ( header->msg_simple != TRUE )	/* Process out of band data */
		ipc_oob_send( header );
	
	/*
	 * Wait for adequate free space to appear on the queue - eventually the
	 * timeout and options fields will have an effect here.
	 * 
	 * The wakeup is driven by hardware interrupts, hence the spl() hackery.
	 */
	s = splhigh();
	while(header->msg_size >= (MSGBUF_SIZE - BYTES_ON_QUEUE(&MSG_QUEUES->FromND)))
		Sleep( &MSG_QUEUES->FromND, priority );
	splx(s);

	status = writequeue( MSGBUF_860_TO_HOST, &MSG_QUEUES->FromND,
		(char *)header, header->msg_size );
	/* Message is enqueued for the host.  Post an interrupt to inform the driver */
	if ( status == SEND_SUCCESS && (MSG_QUEUES->Flags & MSGFLAG_MSG_OUT_AVAIL) )
		ping_driver();
	return( status );
}

/*
 * msg_receive:
 *
 *	Receive a message from the host.  We need to look in two places.  First,
 *	check the Unclaimed message queue.  Second, try waiting for a message on
 *	the Pending queue.
 *
 *	For the moment, we ignore the options and timeout, as we have no
 *	timer capability.  Every msg_receive is a blocking receive.
 */
 msg_return_t
msg_receive( msg_header_t *header, msg_option_t option, msg_timeout_t timeout )
{
	extern struct proc *P;	/* The global current process */
	msg_tab *result;
	queue_t unclaimed_q;
	queue_t pending_q;
	msg_return_t status;
	register int priority = P->p_pri;
	register PortList *p;
	/* Special case: kmalloc() free path for kernel services (pager and whatnot) */
	if ( header->msg_local_port == MSG_QUEUES->service_replyport )
	{
		status = ServiceMsg_receive( header, option, timeout );
		if ( status == RCV_SUCCESS && header->msg_simple != TRUE )
		    ipc_oob_receive( header );
		return status;
	}
		
	/* Select the queue set based on the message destination port */
	p = PPortList;
	while ( p != (PortList *) 0 )/* Linear search, but there are very few entries*/
	{
		if ( p->port == header->msg_local_port )
		{
			unclaimed_q = &p->unclaimed_q;
			pending_q = &p->pending_q;
			break;
		}
		p = p->next;
	}
	if ( p == (PortList *) 0 )
		return RCV_INVALID_PORT;

	/* Is there an unclaimed message for this reader? */
	queue_deq( unclaimed_q, msg_tab *, result );
	if ( result != (msg_tab *)0 )
	{
	    if ( result->data->msg_size <= header->msg_size )
	    {
		bcopy( (char *)result->data, (char *)header, result->data->msg_size );
		kfree( (char *)result->data );
		kfree( (char *)result );
		if ( header->msg_simple != TRUE )
		    ipc_oob_receive( header );
		return( RCV_SUCCESS );
	    }
	    else    /* Message is too large. Leave it on the queue for next time. */
	    {
		bcopy( (char *)result->data, (char *)header, sizeof (msg_header_t) );
	    	queue_preq( unclaimed_q, result );
		return( RCV_TOO_LARGE );
	    }
	}
	/*
	 * No unclaimed messages for us.  Wait on the Pending queue for a message.
	 */
	result = (msg_tab *) kmalloc( sizeof (msg_tab) );
	result->flags = 0;
	result->status = 0;
	result->data = header;
	queue_enq(pending_q, result)

	do
	{
	    Sleep( result, priority );	/* Wait for 'queue' to get a message */
	}
	while( result->flags == 0 );
	status = result->status;
	kfree( (char *)result );	/* Already unlinked from Pending list */
	if ( status == RCV_SUCCESS && header->msg_simple != TRUE )
	    ipc_oob_receive( header );
	return( status );	/* Either RCV_SUCCESS or RCV_TOO_LARGE */
}

/*
 * msg_rpc:
 *
 *	Perform a message send followed by a message receive.
 *
 *	For system service requests, we set up an interlock to insure
 *	that al service RPC requests happen sequentially.  Interleaved
 *	access cannot be supported, due to our inability to allocate
 * 	scratch storage for messages using the service reply path.
 *	Service requests include the mechanisms used to allocate and free
 *	memory.
 */
#define MSG_SERVICE_WANTED	1
#define MSG_SERVICE_BUSY	2

 msg_return_t
msg_rpc( msg_header_t *header, msg_option_t option, msg_size_t rcv_size,
		msg_timeout_t send_timeout, msg_timeout_t rcv_timeout )
{
	msg_return_t status;
	static int service_msg_flag;
	int s;

	if ( header->msg_local_port == MSG_QUEUES->service_replyport )
	{
	    /* Get the message lock */
	    s = splhigh();
	    while ( (service_msg_flag & MSG_SERVICE_BUSY) != 0 )
	    {
	    	service_msg_flag |= MSG_SERVICE_WANTED;
		Sleep( &service_msg_flag, CurrentPriority() );
	    }
	    service_msg_flag |= MSG_SERVICE_BUSY;
	    service_msg_flag &= ~MSG_SERVICE_WANTED;
	    splx( s );

	    status = msg_send( header, option, send_timeout );
	    if ( status != SEND_SUCCESS )
	    {
		    return( status );
	    }
	    header->msg_size = rcv_size;
	    status = msg_receive( header, option, rcv_timeout );

	    /* Clear the message lock */
	    s = splhigh();
	    if ( (service_msg_flag & MSG_SERVICE_WANTED) != 0 )
		Wakeup( &service_msg_flag, CurrentPriority() );
	    service_msg_flag &= ~MSG_SERVICE_BUSY;
	    splx( s );

	    return status;
	}
	else
	{
	    status = msg_send( header, option, send_timeout );
	    if ( status != SEND_SUCCESS )
		    return( status );
	    header->msg_size = rcv_size;
	    return( msg_receive( header, option, rcv_timeout ) );
	}
}



/*
 * Read data off of our circular buffer.
 */
 static void
readqueue( char * buf, NDQueue *q, char *dst, int size )
{
	int cnt;

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
 * Read data off of our circular buffer, but don't bump the tail pointer.
 */
 static void
snoopqueue( char * buf, NDQueue *q, char *dst, int size )
{
	int cnt;

	if ( (cnt = MSGBUF_SIZE - q->Tail) < size )	/* Do copy in two pieces */
	{
		bcopy( &buf[q->Tail], dst, cnt );
		dst += cnt;				/* Bump dst to next byte */
		size -= cnt;
		
		bcopy( buf, dst, size );		/* Copy 2nd half */
	}
	else						/* Do copy in one piece */
		bcopy( &buf[q->Tail], dst, size );	/* Copy off queue */
}

/*
 * Skip data on our circular buffer.
 */
 static void
skipqueue( NDQueue *q, int size )
{
	int cnt;

	if ( (cnt = MSGBUF_SIZE - q->Tail) < size )	/* Do skip in two pieces */
	{
		size -= cnt;
		q->Tail = size;				/* Wrap tail back around */
	}
	else						/* Do skip in one piece */
	{
		q->Tail += size;			/* Bump up tail pointer */
	}
}

/*
 * Write data to our circular buffer.
 */
 static msg_return_t
writequeue( char * buf, NDQueue *q, char *src, int size )
{
	int cnt;
	int s;
#if defined(DEBUG_TRIGGERS)
	int trash;
#endif
	if ( size >= MSGBUF_SIZE )
		return( SEND_MSG_TOO_LARGE );
	
	/* Wait for adequate free space to appear on the queue */
	while( size >= (MSGBUF_SIZE - BYTES_ON_QUEUE(q)) )
		Sleep( q, PUSER );
	s = splhigh();
	if ( (cnt = MSGBUF_SIZE - q->Head) < size )	/* Do copy in two pieces */
	{
		bcopy( src, &buf[q->Head], cnt );
		src += cnt;				/* Bump src to next byte */
		size -= cnt;
		
		bcopy( src, buf, size );		/* Copy 2nd half */
#if defined(DEBUG_TRIGGERS)
		trash = *(ADDR_DAC_PALETTE_PORT);
#endif
		q->Head = size;				/* Wrap head back around */
	}
	else						/* Do copy in one piece */
	{
		bcopy( src, &buf[q->Head], size );	/* Copy off queue */
#if defined(DEBUG_TRIGGERS)
		trash = *(ADDR_DAC_PALETTE_PORT);
#endif
		q->Head += size;			/* Bump up head pointer */
	}
	splx(s);
	return( SEND_SUCCESS );
}





