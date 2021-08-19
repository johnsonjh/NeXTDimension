#include <mach.h>
#include <kernserv/kern_loader_reply.h>
#include <kernserv/kern_loader_types.h>
#include "NDlib.h"

#define WAIT_QUANTUM	5
#define WAIT_MAX	30

kern_return_t get_server_state( port_t, char *, server_state_t * );

#if defined(TEST)
#define DEBUG	1
#endif

#if defined(TEST)
main()
{
	printf( "getting loaded...\n" ); 
	if ( ND_Load_MachDriver() != KERN_SUCCESS )
		exit( 1 );

	printf( "getting unloaded...\n" ); 
	if ( ND_Remove_MachDriver() != KERN_SUCCESS )
		exit( 1 );
	exit( 0 );
}
#endif

 kern_return_t
ND_Load_MachDriver()
{
    port_t	loader_port;
    task_t	kernel_task;
    kern_return_t r;
    server_state_t state;
    int total_wait_time = 0;
    
    /* Get kern_loader's port. */
    r=kern_loader_look_up(&loader_port);
    if (r != KERN_SUCCESS)
    {
#if defined(DEBUG)
	kern_loader_error("Couldn't find kern_loader's port", r);
#endif
	return r;
    }
    /*
     * If we get lucky, we won't need the kernel_task port.
     * Try to continue if task_by_unix_pid() fails.
     */
    r = task_by_unix_pid(task_self(), 0, &kernel_task);
    if (r != KERN_SUCCESS)
    {
#if defined(DEBUG)
	mach_error("cannot get kernel task port", r);
#endif
	kernel_task == TASK_NULL;
    }

    while( 1 )
    {
	r = get_server_state( loader_port, "NextDimension", &state );
	if ( r == KERN_LOADER_UNKNOWN_SERVER )
	{
	    state = Deallocated;	/* Same actions as Deallocated... */
	}
	else if ( r != KERN_SUCCESS )
	{
#if defined(DEBUG)
	    mach_error( "get_server_state", r );
#endif
	    return r;
	}
#if defined(DEBUG)
	printf( "Server state %s\n", server_state_string(state) );
#endif
	switch( state )
	{
	    case Deallocated:	/* Go allocate it! */
	    	/*
		 * For this to work, we must have acquired the kernel
		 * task port, which requires super-user authority.
		 *
		 * All other cases will work without super-user authority.
		 */
		r = kern_loader_add_server( loader_port,
					    kernel_task,
					    ND_SERV_RELOC);
		if ( r != KERN_SUCCESS )
		{
#if defined(DEBUG)
		    mach_error( "kern_loader_add_server", r );
#endif
		    return r;
		}
		/* Fall through */
	    case Allocated:	/* Load it */
		r = kern_loader_load_server(loader_port, ND_SERV_NAME);
		if ( r != KERN_SUCCESS )
		{
#if defined(DEBUG)
		    mach_error( "kern_loader_load_server", r );
#endif
		    return r;
		}
		break;

	    case Allocating:	/* Stand by... */
 	    case Loading:	/* Stand by... */
	    	if ( total_wait_time >= WAIT_MAX )
			return KERN_FAILURE;
	    	total_wait_time += WAIT_QUANTUM;
	    	sleep( WAIT_QUANTUM );
		break;

	    case Loaded:	/* READY */
		return KERN_SUCCESS;

	    case Unloading:	/* trouble... */
	    case Zombie:	/* trouble... */
		return KERN_FAILURE;
	}
    }
}

 kern_return_t
ND_Remove_MachDriver()
{
    port_t	loader_port;
    task_t	kernel_task;
    port_t	server_port;
    kern_return_t r;

    /* Get kern_loader's port. */
    r = kern_loader_look_up(&loader_port);
    if (r != KERN_SUCCESS)
    {
#if defined(DEBUG)
	kern_loader_error("Couldn't find kern_loader's port", r);
#endif
	return r;
    }
    r = task_by_unix_pid(task_self(), 0, &kernel_task);
    if (r != KERN_SUCCESS)
    {
#if defined(DEBUG)
	    mach_error("cannot get kernel task port", r);
#endif
	    return r;
    }

    r = kern_loader_server_task_port(	loader_port,
    					kernel_task,
					ND_SERV_NAME,
					&server_port);
    if (r != KERN_SUCCESS)
    {
#if defined(DEBUG)
	kern_loader_error("Error looking up server port", r);
#endif
	return r;
    }
    r = kern_loader_delete_server(loader_port, kernel_task, ND_SERV_NAME);
    if ( r != KERN_SUCCESS )
    {
#if defined(DEBUG)
	kern_loader_error( "kern_loader_delete_server", r );
#endif
	return r;
    }
    return KERN_SUCCESS;
}

 kern_return_t
get_server_state( port_t loader_port, char * server, server_state_t * sp )
{
	unsigned int			cnt;
	port_name_t			*ports;
	port_name_string_array_t	port_names;
	boolean_t			*advertised;
	server_state_t			state;
	server_reloc_t			relocatable, loadable;
	vm_address_t			load_address;
	vm_size_t			load_size;
	kern_return_t			r;

	/* Get the information. */
	r = kern_loader_server_info(loader_port, PORT_NULL,
		server, &state, &load_address, &load_size,
		relocatable, loadable, &ports, &cnt,
		&port_names, &cnt, &advertised, &cnt);
	
	if ((r != KERN_SUCCESS) && (r != KERN_LOADER_NO_PERMISSION))
	    return r;

	*sp = state;
	return KERN_SUCCESS;
}