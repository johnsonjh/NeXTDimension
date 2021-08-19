/*
 * Copyright 1989 NeXT, Inc.
 *
 * Prototype NextDimension kernel server.
 *
 * This server implements a trivial Mach driver that monitors NextBus devices for
 * interrupts and dispatches a message on receipt of an interrupt.
 */
#include "ND_patches.h"			/* Must be FIRST! */
#include <sys/mig_errors.h>
#include <sys/param.h>
#include <nextdev/slot.h>
#include <next/pcb.h>
#include <next/scr.h>
#include <vm/vm_kern.h>
#include <vm/vm_param.h>
#include <sys/callout.h>
#include "ND_var.h"
#include "ND_types.h"

/*
 * prototypes for kernel functions
 */
void *kalloc(int);
void kfree( void *, int);
void assert_wait(void *, boolean_t);
void timeout(void (*f)(), int, int );
void untimeout(void (*f)(), int);
kern_return_t port_alloc(task_t, port_t *);
task_t task_self();
thread_t kernel_thread(task_t, void (*)());

extern ND_var_t ND_var[];

/*
 *	Port name service: Manage a mapping between an ASCII string and a port_t.
 *
 *	These functions all use a linear search through a linked list.
 *	There should only be 2 or 3 entries, though, so this method is cheap.
 *	If the list gets longer, we should switch to a more efficient method.
 *
 *	Ports are added and removed from the list by ND_Kern_port_allocate()
 *	and ND_Kern_port_deallocate() over in ND_server.c, using the external Mach
 * 	interfaces.  This module uses the kernel internal interface, for access to 
 *	things like vm_move().
 */

 kern_return_t
ND_name_to_port( ND_var_t *sp, char *name, port_t *port_ptr )
{
	ND_port_name *p;
	
	lock_read(sp->lock);
	p = sp->portnames;
	while ( p != (ND_port_name *) 0 )
	{
		if ( p->name != (char *)0 && strcmp( p->name, name ) == 0 )
		{
			*port_ptr = p->port;
			break;
		}
		p = p->next;
	}
	lock_done( sp->lock );
	if ( p == (ND_port_name *) 0 )
		return KERN_NOT_IN_SET;	/* No such port! */

	return KERN_SUCCESS;
}

 kern_return_t
ND_Kern_port_check_in( port_t serv_port, char *name, port_t port )
{
	ND_port_name *p;
	ND_var_t *sp = (ND_var_t *)serv_port;
	
	lock_read(sp->lock);
	p = sp->portnames;
	while ( p != (ND_port_name *) 0 )
	{
		if ( p->port == port )
			break;
		p = p->next;
	}
	lock_done(sp->lock);
	if ( p == (ND_port_name *) 0 )
		return KERN_NOT_IN_SET;	/* No such port! */	
	if ( p->name != (char *) 0 )
		return KERN_NAME_EXISTS;
	lock_write(sp->lock);
	p->name = (char *)kalloc( strlen(name) + 1 );
	strcpy( p->name, name );
	lock_done(sp->lock);
	/* Wake up any one waiting for a new name/port binding to be posted. */
	thread_wakeup( &sp->portnames );
	return KERN_SUCCESS;
}

 kern_return_t
ND_Kern_port_look_up( port_t serv_port, char *name, port_t *port_ptr )
{
	ND_var_t *sp = (ND_var_t *)serv_port;
	
	return ( ND_name_to_port( sp, name, port_ptr ) );
}

 kern_return_t
ND_Kern_port_check_out( port_t serv_port, char *name )
{
	ND_port_name *p;
	ND_var_t *sp = (ND_var_t *)serv_port;
	
	lock_read(sp->lock);
	p = sp->portnames;
	while ( p != (ND_port_name *) 0 )
	{
		if ( p->name != 0 && strcmp( p->name, name ) == 0 )
			break;
		p = p->next;
	}
	lock_done(sp->lock);
	if ( p == (ND_port_name *) 0 )
		return KERN_NOT_IN_SET;	/* No such port! */
	lock_write(sp->lock);
	kfree( p->name, strlen( p->name ) + 1 );
	p->name = (char *)0;
	lock_done(sp->lock);
	return KERN_SUCCESS;
}

/*
 *	VM Initialization:
 *
 *	This function is called from ND_SetOwner() in ND_services.c after a paging 
 *	task is allocated, but before an i860 task is loaded.  Perform any initial
 *	setup work here.
 *
 *	Currently, we set up a zero page with all access restricted.
 */
 void
ND_Kern_config_VM( ND_var_t *sp )
{
	vm_address_t addr = (vm_address_t)0;
	kern_return_t r;
	
	r = vm_allocate( sp->map_pager, &addr, (vm_size_t)PAGE_SIZE, FALSE );
	if ( r == KERN_SUCCESS )
	    vm_protect(sp->map_pager, addr, (vm_size_t)PAGE_SIZE, FALSE, VM_PROT_NONE);
}

/*
 *	VM Management
 *
 *	These functions are used by the i860 to manipulate the state of it's backing
 *	store.  They are basically a layer of glue on top of vm_allocate(),
 *	vm_deallocate(), and vm_protect();
 */

 kern_return_t
ND_Kern_vm_allocate(	port_t		sport,
			vm_address_t	*address,
			vm_size_t	size,
			boolean_t	anywhere)
{
	ND_var_t *sp = (ND_var_t *)sport;
	
	return( vm_allocate( sp->map_pager, address, size, anywhere ) );
}

 kern_return_t
ND_Kern_vm_deallocate( port_t sport, vm_address_t address, vm_size_t size )
{
	ND_var_t	*sp = (ND_var_t *)sport;
	vm_address_t	endaddr = (address + size);

	/* If the deallocated region intersects the pageing cache, invalidate it. */
	if ( sp->npg_at != 0 )
	{
	    if ( endaddr > sp->npg_vaddr && address < (sp->npg_vaddr+sp->npg_size) )
	    {
		vm_deallocate( kernel_map, sp->npg_at, sp->npg_size );
		sp->npg_at = 0;
	    }
	}
	return( vm_deallocate( sp->map_pager, address, size ) );
}

 kern_return_t
ND_Kern_vm_protect(	port_t		sport,
			vm_address_t	address,
			vm_size_t	size,
			boolean_t	set_max,
			vm_prot_t	new_protection )
{
	ND_var_t	*sp = (ND_var_t *)sport;
	vm_address_t	endaddr = (address + size);
	
	/* If the changed region intersects the pageing cache, invalidate it. */
	if ( sp->npg_at != 0 )
	{
	    if ( endaddr > sp->npg_vaddr && address < (sp->npg_vaddr+sp->npg_size) )
	    {
		vm_deallocate( kernel_map, sp->npg_at, sp->npg_size );
		sp->npg_at = 0;
	    }
	}
	return( vm_protect( sp->map_pager, address, size, set_max, new_protection ) );
}

/*
 *	Basic paging support functions.  These tell us what pages to move
 *	between the VM backing store and DRAM on the ND board.
 *
 *	We move (copy) the data from the pager backing store to the kernel.
 *	The data is then wired down and copied to the NextDimension board, 
 *	at the specified physical address.  (The board is transparently mapped
 *	into this thread.)  After the transfer is complete, the copied data is
 *	unwired and deallocated.
 *
 *	To try and keep map fragmentation down, we move large chunks aligned on
 *	64 Kbyte boundries when possible.
 */
#define CHUNK_SIZE	0x10000
#define CHUNK_MASK	(CHUNK_SIZE - 1)
#define round_chunk(a)	(((a)+CHUNK_MASK) & ~CHUNK_MASK)
#define trunc_chunk(a)	((a) & ~CHUNK_MASK)
#define frag_chunk(a)	((a) & CHUNK_MASK)


 kern_return_t
ND_Kern_page_in( port_t sport, vm_movepages_t pagelist, unsigned int pagelistCnt )
{
	ND_var_t *sp = (ND_var_t *)sport;
	vm_address_t physaddr;
	vm_offset_t off;
	vm_address_t pageaddr, destaddr, wireaddr;
	vm_size_t size, moved, tomove, wiresize;
	pmap_t	pmap = vm_map_pmap(sp->map_pager);
	kern_return_t r;
	
	while ( pagelistCnt-- )
	{
	    pageaddr = pagelist->vaddr;
	    /* Find ND address in our virt addr space. */
	    destaddr = CONVERT_PADDR(sp,pagelist->paddr);
	    size = pagelist->size;
	    
	    /*
	     * Wire the range of memory to be copied to ND board.
	     * Try to wire a chunk first.  If we can't get that,
	     * we may be at the start or end of the address space,
	     * and not have a full chunk to use.  Try for the requested
	     * page(s).
	     */
	    wireaddr = trunc_chunk(pageaddr);
	    wiresize = round_chunk(size);
	    if ( vm_map_check_protection(sp->map_pager,
	    				wireaddr,
	    				wireaddr+wiresize,
					VM_PROT_READ ) == FALSE )
	    {
		wireaddr = trunc_page(pageaddr);
		wiresize = round_page(size);
	    }
	    r = vm_map_pageable(sp->map_pager,
	    			wireaddr,
	    			wireaddr+wiresize,
				FALSE);
	    if ( r != KERN_SUCCESS )
	    {
	    	    printf( "ND_Kern_page_in buffer wire fails\n" );
		    return r;
	    }

	    /* Compute starting offset, if any */
	    off = pageaddr & (PAGE_SIZE - 1);
	    moved = 0;
	    while ( moved < size )
	    {
	    	/*
		 * Blocks copied MUST NOT cross page boundries!
		 * Take care to honor leading and trailing frags.
		 */
	    	tomove = PAGE_SIZE - off;
		if ( (tomove + moved) > size )
			tomove = size - moved;
		/* pmap_resident_extract figures in page offset in phys addr */
	    	physaddr = pmap_resident_extract( pmap, pageaddr );
		if ( physaddr == 0 )
		{
		    printf("ND_Kern_page_in: No phys addr on wired page\n");
		    return r;
		}
		copy_from_phys( physaddr, destaddr, tomove );
		pageaddr += tomove;
		destaddr += tomove;
		moved += tomove;
		off = 0;	/* Done with frag */ 
	    }
	    /* Unwire the range of memory updated from the ND board. */
	    r = vm_map_pageable(sp->map_pager,
	    			wireaddr,
	    			wireaddr+wiresize,
				TRUE);
	    if ( r != KERN_SUCCESS )
	    {
	    	    printf( "ND_Kern_page_in buffer unwire fails\n" );
		    return r;
	    }
	    ++pagelist;
	}
	return KERN_SUCCESS;
}

/*
 * Hook to destroy the page cache as part of the shutdown sequence.
 */
 kern_return_t
ND_Kern_destroy_pagecache( ND_var_t *sp )
{
	kern_return_t r;
	return KERN_SUCCESS;
}
/*
 *	Currently, pages on the i860 are smaller than pages on the host.
 *	This requires us to perform a read-modify-write sequence on pageout,
 * 	rather than a more efficient simple write to the backing store.
 *
 *	We move (copy) a block from the pager backing store to the kernel.
 *	The block is then wired down, and updated from the NextDimension
 *	board.  (The board is transparently mapped into this thread.)  After
 *	the transfer is complete, the updated data is unwired and moved back
 *	to the pager backing store.
 *
 *	To try and keep map fragmentation down, we move large chunks aligned on
 *	64 Kbyte boundries when possible.
 */
#include <vm/vm_page.h>		/* For lock and unlock macros. */

 kern_return_t
ND_Kern_page_out( port_t sport, vm_movepages_t pagelist, unsigned int pagelistCnt )
{
	ND_var_t *sp = (ND_var_t *)sport;
	vm_address_t physaddr;
	vm_offset_t off;
	vm_address_t pageaddr, srcaddr, wireaddr;
	vm_size_t size, moved, tomove, wiresize;
	pmap_t	pmap = vm_map_pmap(sp->map_pager);
	vm_page_t	m;
	kern_return_t r;
	
	while ( pagelistCnt-- )
	{
	    pageaddr = pagelist->vaddr;
	    /* Find ND address in our virt addr space. */
	    srcaddr = CONVERT_PADDR(sp,pagelist->paddr);
	    size = pagelist->size;
	    
	    /*
	     * Wire the range of memory to be copied to ND board.
	     * Try to wire a chunk first.  If we can't get that,
	     * we may be at the start or end of the address space,
	     * and not have a full chunk to use.  Try for the requested
	     * page(s).
	     */
	    wireaddr = trunc_chunk(pageaddr);
	    wiresize = round_chunk(size);
	    if ( vm_map_check_protection(sp->map_pager,
	    				wireaddr,
	    				wireaddr+wiresize,
					VM_PROT_WRITE ) == FALSE )
	    {
		wireaddr = trunc_page(pageaddr);
		wiresize = round_page(size);
	    }
	    r = vm_map_pageable(sp->map_pager,
	    			wireaddr,
	    			wireaddr+wiresize,
				FALSE);
	    if ( r != KERN_SUCCESS )
	    {
	    	    printf( "ND_Kern_page_out buffer wire fails\n" );
		    return r;
	    }
	    /* Compute starting offset, if any */
	    off = pageaddr & (PAGE_SIZE - 1);
	    moved = 0;
	    vm_page_lock_queues();
	    while ( moved < size )
	    {
	    	/*
		 * Blocks copied MUST NOT cross page boundries!
		 * Take care to honor leading and trailing frags.
		 */
	    	tomove = PAGE_SIZE - off;
		if ( (tomove + moved) > size )
			tomove = size - moved;
		/* pmap_resident_extract figures in page offset in phys addr */
	    	physaddr = pmap_resident_extract( pmap, pageaddr );
		if ( physaddr == 0 )
		{
		    printf("ND_Kern_page_out: No phys addr on wired page\n");
		    return r;
		}

		/* Modify the physical page. Set the dirty bit for the page. */
		copy_to_phys( srcaddr, physaddr, tomove );
		ND_pmap_set_modify( pmap, trunc_page(pageaddr) );
		/* Flag as modified for the page out daemon */
		m = PHYS_TO_VM_PAGE(trunc_page(physaddr));
		vm_page_set_modified(m);

		pageaddr += tomove;
		srcaddr += tomove;
		moved += tomove;
		off = 0;	/* Done with frag */ 
	    }
	    vm_page_unlock_queues();

	    /* Unwire the range of memory updated from the ND board. */
	    r = vm_map_pageable(sp->map_pager,
	    			wireaddr,
	    			wireaddr+wiresize,
				TRUE);
	    if ( r != KERN_SUCCESS )
	    {
	    	    printf( "ND_Kern_page_out buffer unwire fails\n" );
		    return r;
	    }
	    ++pagelist;
	}
	return KERN_SUCCESS;
}


/*
 *	ND_vm_move:
 *
 *	Move memory from source to destination map, possibly deallocating
 *	the source map reference to the memory.
 *
 *	A shameless ripoff of vm_move, hacked to use only vm_read and
 *	vm_write to move data between the tasks.  The kernel version of these
 *	routines moves data to and from the ipc_soft_map.  Tp perform a task 
 *	to task move, we first vm_read the data from the source map into the
 *	ipc_soft_map.  Then we vm_write the data from the ipc_soft_map to the
 *	destination map.  Finally, we deallocate the space in ipc_soft_map.
 *
 *	In this way, we (slowly) comply with the kernel 'API' for loadable
 *	device drivers.
 *
 *	Parameters are as follows:
 *
 *	src_map		Source address map
 *	src_addr	Address within source map
 *	dst_map		Destination address map
 *	num_bytes	Amount of data (in bytes) to copy/move
 *	src_dealloc	Should source be removed after copy?
 *
 *	Assumes the src and dst maps are not already locked.
 *
 *	If successful, returns destination address in dst_addr.
 */
 kern_return_t
ND_vm_move(src_map,src_addr,dst_map,num_bytes,src_dealloc,dst_addr)
	vm_map_t		src_map;
	register vm_offset_t	src_addr;
	register vm_map_t	dst_map;
	vm_offset_t		num_bytes;
	boolean_t		src_dealloc;
	vm_offset_t		*dst_addr;
{
	register vm_offset_t	src_start;	/* Beginning of region */
	register vm_size_t	src_size;	/* Size of rounded region */
	vm_offset_t		dst_start;	/* destination address */
	vm_offset_t		soft_map_addr;	/* Addr in ipc_soft_map */
	vm_size_t		soft_map_size;	/* Addr in ipc_soft_map */
	extern	vm_map_t	ipc_soft_map;
	register kern_return_t	result;

	if (num_bytes == 0) {
		*dst_addr = 0;
		return KERN_SUCCESS;
	}

	/*
	 *	Page-align the source region
	 */

	src_start = trunc_page(src_addr);
	src_size = round_page(src_addr + num_bytes) - src_start;

	/*
	 *	Allocate a place to put the copy
	 */

	dst_start = (vm_offset_t) 0;
	result = vm_allocate(dst_map, &dst_start, src_size, TRUE);
	if (result != KERN_SUCCESS)
		return result;
	/*
	 * Read from the source map into the ipc_soft_map.
	 */
	result = vm_read( src_map, src_start, src_size,
				&soft_map_addr, &soft_map_size );
	if ( result != KERN_SUCCESS )
	{
		vm_deallocate( dst_map, dst_start, src_size );
		return result;
	}
	/*
	 * Write from the ipc_soft_map into the destination map.
	 * Destroy the intermediate copy in ipc_soft_map.
	 */
	result = vm_write( dst_map, dst_start, soft_map_addr, soft_map_size );
	vm_deallocate( ipc_soft_map, soft_map_addr, soft_map_size );
	if ( result != KERN_SUCCESS )
	{
		vm_deallocate( dst_map, dst_start, src_size );
		return result;
	}
	if ( src_dealloc )
	{
		result = vm_deallocate( src_map, src_start, src_size );
		if ( result != KERN_SUCCESS )
			return result;
	}
	/*
	 *	Return the destination address corresponding to
	 *	the source address given (rather than the front
	 *	of the newly-allocated page).
	 */
	*dst_addr = dst_start + (src_addr - src_start);

	return(result);
}


/*
 *	ND_vm_write:
 *
 *	Write data from the kernel map to the destination map, possibly deallocating
 *	the kernel map reference to the memory.
 *
 *	This is another hack to use only the approved kernel functions vm_read and
 *	vm_write to move data between the tasks.  The kernel version of these
 *	routines moves data to and from the ipc_soft_map.  Tp perform a task 
 *	to task write, we first vm_read the data from the kernel map into the
 *	ipc_soft_map.  Then we vm_write the data from the ipc_soft_map to the
 *	destination map.  Finally, we deallocate the space in ipc_soft_map.
 *
 *	Space in the destination map must already be allocated.
 *
 *	In this way, we (slowly) comply with the kernel 'API' for loadable
 *	device drivers.
 *
 *	Parameters are as follows:
 *
 *	dst_map		Destination address map (on page boundry)
 *	dst_addr	Address within destination map
 *	src_addr	Address within source map (on page boundry)
 *	num_bytes	Amount of data (in bytes) to copy/move (multiple of PAGE_SIZE)
 *
 *	If successful, returns destination address in dst_addr.
 */
 kern_return_t
ND_vm_write(dst_map,dst_addr,src_addr,num_bytes)
	register vm_map_t	dst_map;
	register vm_offset_t	dst_addr;
	register vm_offset_t	src_addr;
	vm_offset_t		num_bytes;
{
	vm_offset_t		soft_map_addr;	/* Addr in ipc_soft_map */
	vm_size_t		soft_map_size;	/* Addr in ipc_soft_map */
	extern	vm_map_t	ipc_soft_map;
	register kern_return_t	result;

	if (num_bytes == 0) {
		return KERN_SUCCESS;
	}

	/*
	 * Read from the kernel_map into the ipc_soft_map.
	 * This allocates space in the ipc_soft_map.
	 */
	result = vm_read( kernel_map, src_addr, num_bytes,
				&soft_map_addr, &soft_map_size );
	if ( result != KERN_SUCCESS )
		return result;
	/*
	 * Write from the ipc_soft_map into the destination map.
	 * Destroy the intermediate copy in ipc_soft_map.
	 */
	result = vm_write( dst_map, dst_addr, soft_map_addr, soft_map_size );
	vm_deallocate( ipc_soft_map, soft_map_addr, soft_map_size );

	return result;
}


