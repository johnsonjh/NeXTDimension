#include <stdio.h>
#include <mach.h>
#include <stdio.h>
#include <servers/netname.h>
#include <sys/message.h>
#include "ND/NDlib.h"
#include <libc.h>


ND_var_p	_ND_Handles_[NUMHANDLES];

 kern_return_t
ND_Open(port_t dev_port, int slot )
{	
	kern_return_t	r;
	task_t task = task_self();
	ND_var_p handle;

	if ( SLOT_INVALID(slot) )
		return KERN_INVALID_ARGUMENT;
	
	if ( _ND_Handles_[SlotToHandleIndex(slot)] != (ND_var_p)0 )	/* In use */
	{
		if ( dev_port == _ND_Handles_[SlotToHandleIndex(slot)]->dev_port )
			return KERN_SUCCESS;	/* Already open */
		else
			return KERN_INVALID_ARGUMENT; /* Funny port - cannot re-open */
	}
	/*
	 * Not open yet, and no storage allocated.
	 */
	if ( (handle = (ND_var_p)calloc( 1, sizeof (ND_var_t) )) == (ND_var_p)0 )
		return KERN_RESOURCE_SHORTAGE;		/* out of memory???? */
		
	_ND_Handles_[SlotToHandleIndex(slot)] = handle;
	
	handle->slot = slot;
	handle->t_flag = 0;
	handle->dev_port = dev_port;
   
	if ( (r = port_allocate( task, &handle->negotiation_port)) != KERN_SUCCESS)
	    goto bad;

	if ( (r = port_allocate( task, &handle->owner_port)) != KERN_SUCCESS)
	    goto bad;

	/*
	 * Get ownership of the board.
	 */
	r = ND_SetOwner(dev_port, slot, handle->owner_port, &handle->negotiation_port);
	if ( r != KERN_SUCCESS )
	    goto bad;
	/*
	 * Map in the portions of the hardware that we need to work.
	 */
	r = ND_MapCSR( dev_port, handle->owner_port, task, slot,
				&handle->csr_addr, &handle->csr_size );
	if ( r != KERN_SUCCESS )
	    goto bad;

	r = ND_MapGlobals( dev_port, handle->owner_port, task, slot,
				&handle->globals_addr, &handle->globals_size );
	if ( r != KERN_SUCCESS )
	    goto bad;
	
	r = ND_MapHostToNDbuffer( dev_port, handle->owner_port, task, slot,
				&handle->globals_addr, &handle->globals_size );
	if ( r != KERN_SUCCESS )
	    goto bad;

	/*
	 * Get the kernel port to be used for communications with the i860 code.
	 */
	r = ND_GetKernelPort( dev_port, slot, &handle->kernel_port );
	if ( r != KERN_SUCCESS )
	    goto bad;

	r = ND_PortTranslate( dev_port, handle->kernel_port,&handle->NDkernel_port );
	if ( r != KERN_SUCCESS )
	    goto bad;

	return KERN_SUCCESS;
bad:
	mach_error( "ND_Open", r );
	ND_Close( dev_port, slot );
	return r;
}

 kern_return_t
ND_GetOwnerPort( port_t dev_port, int unit, port_t *owner_port )
{
	int i;
	
	if ( SLOT_INVALID(unit) ) 
		return KERN_INVALID_ARGUMENT;
	i = SlotToHandleIndex(unit);
	if ( _ND_Handles_[i] == (ND_var_p)0 )
	{
		*owner_port = PORT_NULL;
		return KERN_NOT_IN_SET;
	}
	if ( _ND_Handles_[i]->dev_port != dev_port )
	{
		*owner_port = PORT_NULL;
		return KERN_NO_ACCESS;
	}
	
	*owner_port = _ND_Handles_[i]->owner_port;
	return KERN_SUCCESS;
}

 void
ND_Close( port_t dev_port, int unit )
{
	task_t task = task_self();
	ND_var_p handle = (ND_var_p) 0;
	int i;

	if ( SLOT_INVALID(unit) ) 
		return;
	i = SlotToHandleIndex(unit);
	if ( _ND_Handles_[i] && _ND_Handles_[i]->dev_port == dev_port )
		handle = _ND_Handles_[i];
	else
		return;

	/* Zap any active mapping */
	if ( handle->csr_addr != (vm_address_t) 0 )
	    (void) vm_deallocate( task, handle->csr_addr, handle->csr_size );
	if ( handle->fb_addr != (vm_address_t) 0 )
	    (void) vm_deallocate( task, handle->fb_addr, handle->fb_size );
	if ( handle->globals_addr != (vm_address_t) 0 )
	    (void) vm_deallocate( task, handle->globals_addr, handle->globals_size );
	if ( handle->dram_addr != (vm_address_t) 0 )
	    (void) vm_deallocate( task, handle->dram_addr, handle->dram_size );
	if ( handle->host_to_ND_addr != (vm_address_t) 0 )
	    (void)vm_deallocate(task,handle->host_to_ND_addr,handle->host_to_ND_size);
	/*
	 * Destroy the ports.
	 */
	handle->dev_port = PORT_NULL;	/* We didn't create it, so don't dealloc it. */
	
	if ( handle->owner_port )	/* Shuts down kernel services for board. */
		port_deallocate( task, handle->owner_port );
	if ( handle->negotiation_port )
		port_deallocate( task, handle->negotiation_port );
	if ( handle->kernel_port )
		port_deallocate( task, handle->kernel_port );

	_ND_Handles_[SlotToHandleIndex(handle->slot)] = (ND_var_p) 0;
	free( handle );
}


