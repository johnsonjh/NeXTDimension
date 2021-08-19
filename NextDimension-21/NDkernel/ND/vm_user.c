#include <kern/mach_types.h>
#include "i860/proc.h"
#include "i860/vm_types.h"
#include "vm/vm_param.h"
#include "vm/vm_kern.h"
#include "ND/NDmsg.h"
#include "ND/NDreg.h"
#include <sys/port.h>
#include <sys/kern_return.h>

#define ServicePort	(MSG_QUEUES->service_port)
#define ReplyPort	(MSG_QUEUES->service_replyport)

extern struct map *coremap;	/* declared in param.c */

#define HOST_PAGE_SIZE	0x2000
#define HOST_PAGE_MASK	(HOST_PAGE_SIZE - 1)


/*
 * Virtual Memory allocation and deallocation:
 *
 *	All VM is allocated on the host, and added to the backing store task that the 
 *	host driver manages for us.
 */
 kern_return_t
vm_allocate(	port_t task,
		vm_address_t	*address,
		vm_size_t	size,
		boolean_t	anywhere)
{
	kern_return_t r;
	vm_offset_t off;
	vm_address_t addr;

	if ( task != ServicePort )
		return KERN_INVALID_ARGUMENT;

	if ( ! anywhere )
	{
	    addr = *address & ~HOST_PAGE_MASK;
	    off = *address & HOST_PAGE_MASK;
	}
	size = (size + HOST_PAGE_MASK) & ~HOST_PAGE_MASK;
	r = ND_Kern_vm_allocate(task, ReplyPort, &addr, size, anywhere);
	if ( r != KERN_SUCCESS )
		return r;

	if ( ! anywhere )
		*address = addr + off;
	else
		*address = addr;

	/* Set up pmap for new memory. */
	pmap_add(	current_pmap(),
			trunc_page(*address),
			round_page(*address + size),
			VM_PROT_DEFAULT,
			FALSE ); 	/* No backing store - zero fill */
	return KERN_SUCCESS;
}

 kern_return_t
vm_deallocate( port_t task, vm_address_t address, vm_size_t size )
{
	kern_return_t r;
	if ( task != ServicePort )
		return KERN_INVALID_ARGUMENT;


	address = address & ~HOST_PAGE_MASK;
	size = (size + HOST_PAGE_MASK) & ~HOST_PAGE_MASK;

	r = ND_Kern_vm_deallocate(task, ReplyPort, address, size);
	if ( r == KERN_SUCCESS )
	{
	    pmap_remove(current_pmap(),
			trunc_page(address),
			round_page(address + size) ); 
	}
	return r;
}

 kern_return_t
vm_protect(	port_t		task,
		vm_address_t	address,
		vm_size_t	size,
		boolean_t	set_max,
		vm_prot_t	new_prot )
{
	kern_return_t r;
	if ( task != ServicePort )
		return KERN_INVALID_ARGUMENT;
	return (ND_Kern_vm_protect(task, ReplyPort, address, size, set_max, new_prot));
}

/*
 * vm_map_hardware:
 *
 *	Map physical addresses to a virtual address range.
 */
 kern_return_t
vm_map_hardware(	port_t task,
		vm_address_t	*address,
		vm_address_t	paddr,
		vm_size_t	size,
		boolean_t	anywhere)
{
	kern_return_t r;
	vm_address_t vaddr, addr;
	vm_offset_t off;

	if ( task != ServicePort )
		return KERN_INVALID_ARGUMENT;

	if ( ! anywhere )
	{
	    addr = *address & ~HOST_PAGE_MASK;
	    off = *address & HOST_PAGE_MASK;
	}
	size = (size + HOST_PAGE_MASK) & ~HOST_PAGE_MASK;
	r = ND_Kern_vm_allocate(task, ReplyPort, &addr, size, anywhere);
	if ( r != KERN_SUCCESS )
		return r;

	if ( ! anywhere )
		*address = addr + off;
	else
		*address = addr;

	/* Map physical memory onto the allocated virtual memory. */
	paddr = trunc_page(paddr);
	
	for (	vaddr = addr;
		    vaddr < (addr + size);
		    vaddr += PAGE_SIZE, paddr += PAGE_SIZE )
	{
	    pmap_enter( current_pmap(), vaddr, paddr, VM_PROT_DEFAULT, TRUE );
	}

	return KERN_SUCCESS;
}

/*
 * Allocate a wired down region of physical memory, mapped into a
 * range of virtual addresses.  The memory is cacheable by default.
 */
 kern_return_t
vm_allocate_wired(	port_t task,
		vm_address_t	*address,
		vm_address_t	*paddress,
		vm_size_t	size,
		boolean_t	anywhere)
{
	kern_return_t r;
	vm_offset_t vaddr, paddr;
	unsigned npages, page;
	
	if ( task != ServicePort )
		return KERN_INVALID_ARGUMENT;

	/* Allocate a contigious block of physical memory. */
	npages = btop( round_page(size) );	
	if ( (page = rmalloc(coremap, npages)) == 0 )
		return( KERN_RESOURCE_SHORTAGE );
	
	/* Extract full hardware address of memory. */
	paddr = kvtophys( ptob(page) );

	/* Get a block of virtual addresses to hols the data. */
	r = ND_Kern_vm_allocate(task, ReplyPort, address, size, anywhere);
	if ( r != KERN_SUCCESS )
	{
		rmfree( coremap, npages, page );
		return r;
	}
	
	/* We're going to succeed. Fill in the hardware address for return. */
	*paddress = paddr;
	
	/* Enter physical mem pages as wired. */
	for (	vaddr = *address;
		    vaddr < (*address + size);
		    vaddr += PAGE_SIZE, paddr += PAGE_SIZE )
	{
	    pmap_enter( current_pmap(), vaddr, paddr, VM_PROT_DEFAULT, TRUE );
	}

	return KERN_SUCCESS;
}

/*
 * vm_cacheable():
 *
 *	Set the cache mode for a range of addresses in a task.
 */
 kern_return_t
vm_cacheable(	port_t task,
		vm_address_t	address,
		vm_size_t	size,
		boolean_t	cache)
{
	    pmap_cache_range( current_pmap(), cache, address, address + size );
	    return KERN_SUCCESS;
}
