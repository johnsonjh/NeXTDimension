#include <kern/mach_types.h>
#include "i860/proc.h"
#include "i860/vm_types.h"
#include "vm/vm_param.h"
#include "vm/vm_kern.h"
#include "ND/NDmsg.h"
#include "ND/NDreg.h"
#include <sys/port.h>
#include <sys/kern_return.h>

extern struct map *coremap;	/* declared in param.c */
extern unsigned rmalloc( struct map *, unsigned );
extern void rmfree(struct map *, unsigned, unsigned);

vm_map_t kernel_map;
 
/*
 * Kernel services related to the VM system.
 *
 * This module contains the code to allocate pages of memory, mapped into the kernel
 * address space at startup, for use in constructing page table entries and related
 * objects.  All allocated memory is guaranteed to be contiguous in physical memory,
 * too, so the video DMA support can use this too.  Whatever map is passed in should
 * deal in pages.  kernel_map is appropriate for getting wired down storage.
 */
 void
kmem_init()
{
	kernel_map = coremap;
}

 vm_offset_t
kmem_alloc( vm_map_t mp, vm_size_t size )
{
	unsigned page;
	unsigned npages;
	vm_offset_t addr;
	
	npages = btop( round_page(size) );
	
	while ( (page = rmalloc(mp, npages)) == 0 )
	{
	    if ( mp == kernel_map )
	    {
		/* Wake page out daemon and sleep waiting for free pages */
		Wakeup( &mp->m_name );
		Sleep( mp, CurrentPriority() );
	    }
	    else
	    	return( (vm_offset_t) 0 );
	}
	addr = (vm_offset_t) ptob(page);
	/*
	 *Once we have a cleaned page queue, we will allocate preferentially off that.
	 */
	bzero( addr, ptob(npages) );
	return( (vm_offset_t) ptob(page) );
}

 void
kmem_free(  vm_map_t mp, vm_size_t size, vm_offset_t addr )
{
	unsigned page, npages;
	
	page = btop( trunc_page( addr ) );
	npages = btop( round_page(size) );
	rmfree( mp, npages, page );
}