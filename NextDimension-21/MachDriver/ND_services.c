/*
 * Copyright 1989 NeXT, Inc.
 *
 * Prototype NextDimension kernel server.
 *
 *	This module contains service functions called out from the MIG ND_server()
 *	interface.
 *
 */
#include "ND_patches.h"			/* Must be FIRST! */
#include <kernserv/kern_server_types.h>	/* Caution: imports mach_user_internal.h */
#include <nextdev/slot.h>
#include <next/pcb.h>
#include <next/scr.h>
#include <vm/vm_kern.h>
#include <sys/callout.h>
#include "ND_var.h"

/*
 * Instance variable used by the kern_server, defined in kernserv/kern_server_types.h.
 */
extern kern_server_t instance;		/* Declared in ND_server.c */

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
 * Aquire the device.
 * The owner port is registered as the device owner.  When we detect
 * that the port has gone away (through a notify message when receive
 * rights to the owner port are deleted) the device will be reset.
 * The Negotiation port is retained and returned for unsuccessful attempts
 * to aquire ownership.
 * Re-specifying ownership causes the device to be reset.
 */
kern_return_t ND_SetOwner (
	port_t		nd_port,
	int		unit,
	port_t		owner_port,
	port_t		*negotiation_port)
{
	ND_var_t	*sp;
	kern_return_t 	r;

	/*
	 * Ensure that arg makes sense.
	 */
	if ( UNIT_INVALID( unit ) )
		return KERN_INVALID_ARGUMENT;
	sp = &ND_var[UNIT_TO_INDEX(unit)];
	
	if ( sp->present == FALSE )
		return KERN_INVALID_ADDRESS;

	if (sp->owner_port == owner_port) {
		/*
		 * Device reset operation.  Should we do anything???
		 */
		sp->negotiation_port = *negotiation_port;
	} 
	else if (sp->owner_port != PORT_NULL) {
		/*
		 * An owner already exists, return the negotiation port
		 * to the caller.
		 */
		*negotiation_port = sp->negotiation_port;
		return KERN_NO_ACCESS;
	}
	/*
	 * Record ownership information
	 */
	sp->attached = TRUE;
	sp->owner_port = owner_port;
	sp->negotiation_port = *negotiation_port;
	r = kern_serv_notify(	&instance,
				kern_serv_notify_port(&instance),
				owner_port);
	if ( r != KERN_SUCCESS )
		printf( "kern_serv_notify returns %d\n", r );
	/*
	 * Allocate and initialize transmit and receive ports,
	 * pager task object, and related items.
	 */
	if ( (r = ND_allocate_resources( sp )) != KERN_SUCCESS )
		return r;

	ND_Kern_config_VM( sp );	/* Configure the VM system for operation. */
	
	ND_init_message_system( sp ); /* Configure the message system for operation. */
	/*
	 * Start the service threads.
	 */
	ND_start_services( sp );
	return KERN_SUCCESS;
}

kern_return_t ND_SetDebug (
	port_t ND_port,
	int	unit,
	port_t debug_port )
{
	ND_var_t	*sp;
	kern_return_t	r;
	
	/*
	 * Ensure that arg makes sense.
	 */
	if ( UNIT_INVALID( unit ) )
		return KERN_INVALID_ARGUMENT;
	sp = &ND_var[UNIT_TO_INDEX(unit)];
	if ( sp->debug_port != PORT_NULL && sp->debug_port != debug_port )
	{
		return KERN_NO_ACCESS;
	}		
	sp->debug_port = debug_port;
	r = kern_serv_notify(	&instance,
				kern_serv_notify_port(&instance),
				debug_port);
	if ( r != KERN_SUCCESS )
		printf( "kern_serv_notify returns %d\n", r );
		
	ND_update_message_system( sp );
	return KERN_SUCCESS;
}

/*
 * Receive a buffer of ASCII to be forwarded as console input to the ND board.
 */
kern_return_t ND_ConsoleInput (
	port_t		nd_port,
	int		unit,
	char *		data,
	int		length)
{
	ND_var_t	*sp;
	/*
	 * Ensure that arg makes sense.
	 */
	if ( UNIT_INVALID( unit ) )
		return KERN_INVALID_ARGUMENT;
	sp = &ND_var[UNIT_TO_INDEX(unit)];

	if ( sp->attached == FALSE )
		return KERN_INVALID_ADDRESS;
	
	if ( length )	/* Round up to int size! */
		bcopy( data, ND_CONIO_IN_BUF( sp ),  (length + 3) & ~3 );
	ND_CONIO_IN_COUNT( sp ) = length;
	ND_CONIO_MSG_ID( sp ) = -1;	/* Handshake so ND code knows data is avail. */

	return KERN_SUCCESS;
}

/*
 * ND memory mapping functions are in ND_map_hw.c.
 */


kern_return_t ND_PortTranslate (
	port_t ND_port,
	port_t app_port,
	int *kernel_value )
{
	*kernel_value = (int)app_port;
	return KERN_SUCCESS;
}

kern_return_t ND_ResetMessageSystem (
	port_t	ND_port,
	int	unit,
	port_t	owner_port )
{
	ND_var_t	*sp;

	/*
	 * Ensure that arg makes sense.
	 */
	if ( UNIT_INVALID( unit ) )
		return KERN_INVALID_ARGUMENT;
	sp = &ND_var[UNIT_TO_INDEX(unit)];
	
	if ( sp->present == FALSE )
		return KERN_INVALID_ADDRESS;

	ND_init_message_system( sp );
	return KERN_SUCCESS;
}

/* ND_RegisterThyself is in ND_cursor.c */

/*
 * Get the port to be used for communication with the ND kernel.
 */
kern_return_t ND_GetKernelPort(
	port_t	ND_port,
	int	unit,
	port_t	*kernel_port )
{
	ND_var_t	*sp;

	/*
	 * Ensure that arg makes sense.
	 */
	if ( UNIT_INVALID( unit ) )
		return KERN_INVALID_ARGUMENT;
	sp = &ND_var[UNIT_TO_INDEX(unit)];
	
	if ( sp->attached == FALSE )
		return KERN_INVALID_ADDRESS;

	if ( sp->kernel_port == PORT_NULL )
	    return KERN_NOT_IN_SET;	 /* Bogus, but the error message makes sense.*/
	*kernel_port = sp->kernel_port;
	return KERN_SUCCESS;
}

/*
 * Get a bit pattern indicating which slots have ND boards in them.
 */
kern_return_t ND_GetBoardList(
	port_t	ND_port,
	int	*BoardList )
{
	int i;
	int result = 0;
	
	for ( i = 0; i < SLOTCOUNT; ++i )
	{
		if ( ND_var[i].present )
			result |= (1 << ND_var[i].slot);
	}
	*BoardList = result;
	return KERN_SUCCESS;
}

/*
 * Get the port to be used for communication with a thread on the ND board.
 */
kern_return_t ND_Port_look_up (
	port_t	ND_port,
	int	unit,
	char	*name,
	port_t	*port_ptr )
{
	ND_var_t	*sp;
	kern_return_t	r;
	extern int	hz;
	int		s;
	/*
	 * Ensure that arg makes sense.
	 */
	if ( UNIT_INVALID( unit ) )
		return KERN_INVALID_ARGUMENT;
	sp = &ND_var[UNIT_TO_INDEX(unit)];
	
	if ( sp->attached == FALSE )
		return KERN_INVALID_ADDRESS;

	return( ND_name_to_port(sp,name,port_ptr) );
}
