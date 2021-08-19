#include "i860/proc.h"
#include "ND/NDmsg.h"
#include "ND/NDreg.h"
#include <sys/port.h>
#include <sys/kern_return.h>

#define ServicePort	(MSG_QUEUES->service_port)
#define ReplyPort	(MSG_QUEUES->service_replyport)

/* For folks who did include mach_init.h... */
port_t	task_self_;	/* task_self() defined to be this! */


/* Called early in startup to let us get configured. */
 void
InitPorts()
{
	task_self_ = ServicePort;
}

/* Stuff to fetch well-known ports within the ND system. */

/* For folks who didn't include mach_init.h... */
 port_t
task_self()
{
	return ServicePort;
}

 port_t
TaskReceivePort()
{
	return( MSG_QUEUES->kernel_port );	/* This WILL change.... */
}

 port_t
KernelDebugPort()
{
	return( MSG_QUEUES->debug_port );
}

 int
KernelDebugEnabled()
{
	return( MSG_QUEUES->debug_port != PORT_NULL );
}

/*
 * Register and unregister ports with the host NextDimension name server.
 */
 kern_return_t
ND_Port_check_in( port_t task, char *name, port_t port )
{
	if ( task != ServicePort )
		return KERN_INVALID_ARGUMENT;
	return (ND_Kern_port_check_in(task, ReplyPort, name ,port));
}

 kern_return_t
ND_Port_check_out( port_t task, char *name )
{
	if ( task != ServicePort )
		return KERN_INVALID_ARGUMENT;
	return (ND_Kern_port_check_out(task, ReplyPort, name));
}

 kern_return_t
ND_Port_look_up( port_t task, char *name, port_t *port_ptr )
{
	if ( task != ServicePort )
		return KERN_INVALID_ARGUMENT;
	return(ND_Kern_port_look_up(task, ReplyPort, name, port_ptr));
}

/*
 * Port allocation and deallocation:
 *
 *	All ports are allocated on the host, and added to the port set that the 
 *	host driver listens on for us.  We add or delete the ports from the
 *	set that our message system will listen on.
 */
 kern_return_t
port_allocate( port_t task, port_t *port_ptr )
{
	kern_return_t r;
	if ( task != ServicePort )
		return KERN_INVALID_ARGUMENT;
	r = ND_Kern_port_allocate(task, ReplyPort, port_ptr);
	if ( r != KERN_SUCCESS )
		return r;
	AddPortToList( *port_ptr );	/* Tell message system about the port */
	return KERN_SUCCESS;
}

 kern_return_t
port_deallocate( port_t task, port_t port )
{
	kern_return_t r;
	if ( task != ServicePort )
		return KERN_INVALID_ARGUMENT;
	DeletePortFromList( port );	/* Remove from message system */
	return (ND_Kern_port_deallocate(task, ReplyPort, port));
}
