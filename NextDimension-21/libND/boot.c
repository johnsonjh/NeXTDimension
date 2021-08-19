#include <stdio.h>
#include <sys/errno.h>
#include <sys/types.h>
#include <sys/file.h>
#include "ND/NDlib.h"
#include <libc.h>

 kern_return_t
ND_BootKernel( port_t dev_port, int slot, char *file )
{
    ND_var_p handle = (ND_var_p) 0;
    kern_return_t	r;
    int i;
    task_t task = task_self();
    
    if ( (r = ND_Open( dev_port, slot )) != KERN_SUCCESS )
    	return r;

    handle = _ND_Handles_[SlotToHandleIndex(slot)];
    r = ND_MapBootDRAM( dev_port, handle->owner_port, task, slot,
				&handle->dram_addr, &handle->dram_size );
    if ( r != KERN_SUCCESS )
    {
    	ND_Close( dev_port, slot );
	return r;
    }
#if defined(PROTOTYPE)
    *ADDR_NDP_CSR(handle) = NDCSR_RESET860;   // Halt the processor
#else /* !PROTOTYPE */
    *ADDR_NDP_CSR0(handle) = NDCSR_RESET860;
#endif /* !PROTOTYPE */
    
    if ( NDLoadCode( handle, file ) == -1 ) // Load code
    {
	ND_Close( dev_port, slot );
	return KERN_FAILURE;
    }
#if defined(PROTOTYPE)
    *ADDR_NDP_CSR(handle) = 0;	/* Clear reset, interrupts */
#else /* !PROTOTYPE */
    // Post a bogus host interrupt as the handshake
    ND_INT860(handle);
#endif /* !PROTOTYPE */
    (void) vm_deallocate( task, handle->dram_addr, handle->dram_size );
    handle->dram_addr = (vm_address_t) 0;
    return KERN_SUCCESS;
}

 kern_return_t
ND_BootKernelFromSect(	port_t dev_port, int slot,
			struct mach_header *mhp, char *seg, char *sect )
{
    ND_var_p handle = (ND_var_p) 0;
    kern_return_t	r;
    int i;
    task_t task = task_self();
    
    if ( (r = ND_Open( dev_port, slot )) != KERN_SUCCESS )
    	return r;

    handle = _ND_Handles_[SlotToHandleIndex(slot)];
    r = ND_MapBootDRAM( dev_port, handle->owner_port, task, slot,
				&handle->dram_addr, &handle->dram_size );
    if ( r != KERN_SUCCESS )
    {
    	ND_Close( dev_port, slot );
	return r;
    }
    
#if defined(PROTOTYPE)
    *ADDR_NDP_CSR(handle) = NDCSR_RESET860;   // Halt the processor
#else /* !PROTOTYPE */
    *ADDR_NDP_CSR0(handle) = NDCSR_RESET860;
#endif /* !PROTOTYPE */
    
    if ( NDLoadCodeFromSect( handle, mhp, seg, sect ) == -1 ) // Load code
    {
	ND_Close( dev_port, slot );
	return KERN_FAILURE;
    }
#if defined(PROTOTYPE)
    *ADDR_NDP_CSR(handle) = 0;	/* Clear reset, interrupts */
#else /* !PROTOTYPE */
    // Post a bogus host interrupt as the handshake
    ND_INT860(handle);
#endif /* !PROTOTYPE */
    (void) vm_deallocate( task, handle->dram_addr, handle->dram_size );
    handle->dram_addr = (vm_address_t) 0;
    return KERN_SUCCESS;
}


