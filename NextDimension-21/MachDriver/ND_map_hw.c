/*
 * Copyright 1990 NeXT, Inc.
 *
 * Prototype NextDimension loadable kernel server.
 *
 * 	This module contains functions which deal with mapping hardware into
 *	virtual address spaces.
 */
#include "ND_patches.h"			/* Must be FIRST! */
#include <kern/kern_port.h>
#include <nextdev/slot.h>
#include <next/pcb.h>
#include <next/mmu.h>
#include <next/scr.h>
#include <vm/vm_kern.h>
#include <vm/vm_param.h>
#include "ND_var.h"


extern ND_var_t ND_var[];

static kern_return_t ND_ioaccess (vm_map_t, vm_offset_t, vm_offset_t, vm_size_t);
static kern_return_t validate_access( port_t, int );
/*
 * Special purpose mappers for interesting things the user might need access to.
 */
 kern_return_t
ND_MapFramebuffer(
	port_t ND_port,
	port_t owner_port,
	port_t task_port,
	int unit,
	vm_offset_t *vaddrp,
	vm_size_t *size )
{
	kern_return_t	r;
	ND_var_t *sp;

	if ( (r = validate_access( owner_port, unit )) != KERN_SUCCESS )
		return r;
	*size = round_page( ND_FB_SIZE );
	r = ND_vm_mapper( ND_port, owner_port, task_port,
			  vaddrp, ND_FB_PHYS_ADDR(UNIT_TO_INDEX(unit)), ND_FB_SIZE );
	return r;
}

 kern_return_t
ND_MapCSR(
	port_t ND_port,
	port_t owner_port,
	port_t task_port,
	int unit,
	vm_offset_t *vaddrp,
	vm_size_t *size )
{
	kern_return_t	r;

	if ( (r = validate_access( owner_port, unit )) != KERN_SUCCESS )
		return r;

	*size = round_page( ND_CSR_SIZE );
	r = ND_vm_mapper( ND_port, owner_port, task_port,
			vaddrp, ND_CSR_PHYS_ADDR(UNIT_TO_INDEX(unit)), ND_CSR_SIZE );
	return r;
}

#if ! defined(PROTOTYPE)
 kern_return_t
ND_MapBT463(
	port_t ND_port,
	port_t owner_port,
	port_t task_port,
	int unit,
	vm_offset_t *vaddrp,
	vm_size_t *size )
{
	kern_return_t	r;

	if ( (r = validate_access( owner_port, unit )) != KERN_SUCCESS )
		return r;

	*size = round_page( ND_BT_SIZE );
	r = ND_vm_mapper( ND_port, owner_port, task_port,
			vaddrp, ND_BT_PHYS_ADDR(UNIT_TO_INDEX(unit)), ND_BT_SIZE );
	return r;
}
#endif

 kern_return_t
ND_MapDRAM(
	port_t ND_port,
	port_t owner_port,
	port_t task_port,
	int unit,
	vm_offset_t *vaddrp,
	vm_size_t *size )
{
	kern_return_t	r;

	if ( (r = validate_access( owner_port, unit )) != KERN_SUCCESS )
		return r;

	*size = round_page( ND_DRAM_SIZE );
	r = ND_vm_mapper( ND_port, owner_port, task_port,
			vaddrp, ND_DRAM_PHYS_ADDR(UNIT_TO_INDEX(unit)), ND_DRAM_SIZE );
	return r;
}

/* Map in the 1st megabyte of DRAM, for the purpose of booting a kernel */
 kern_return_t
ND_MapBootDRAM(
	port_t ND_port,
	port_t owner_port,
	port_t task_port,
	int unit,
	vm_offset_t *vaddrp,
	vm_size_t *size )
{
	kern_return_t	r;

	if ( (r = validate_access( owner_port, unit )) != KERN_SUCCESS )
		return r;

	*size = round_page( 0x100000 );	/* 1 Mbyte */
	r = ND_vm_mapper( ND_port, owner_port, task_port,
			vaddrp, ND_DRAM_PHYS_ADDR(UNIT_TO_INDEX(unit)), 0x100000 );
	return r;
}

 kern_return_t
ND_MapGlobals(
	port_t ND_port,
	port_t owner_port,
	port_t task_port,
	int unit,
	vm_offset_t *vaddrp,
	vm_size_t *size )
{
	kern_return_t	r;

	if ( (r = validate_access( owner_port, unit )) != KERN_SUCCESS )
		return r;

	*size = round_page( ND_GLOBALS_SIZE );
	r = ND_vm_mapper( ND_port, owner_port, task_port,
		vaddrp, ND_GLOBALS_PHYS_ADDR(UNIT_TO_INDEX(unit)), ND_GLOBALS_SIZE );
	return r;
}

 kern_return_t
ND_MapHostToNDbuffer(
	port_t ND_port,
	port_t owner_port,
	port_t task_port,
	int unit,
	vm_offset_t *vaddrp,
	vm_size_t *size )
{
	kern_return_t	r;
	ND_var_t *sp;

	if ( (r = validate_access( owner_port, unit )) != KERN_SUCCESS )
		return r;
	
	sp = &ND_var[UNIT_TO_INDEX(unit)];

	*size = round_page( MSGBUF_SIZE );
	r = ND_vm_mapper( ND_port, owner_port, task_port,
		vaddrp, MSGBUF_HOST_TO_860(sp), MSGBUF_SIZE );
	return r;
}

/*
 * ND_vm_mapper:
 *
 *	Given a task port, allocate a chunk of VM in that task and map
 * 	the given physical addresses into the allocated VM.
 */
 kern_return_t
ND_vm_mapper(
	port_t ND_port,
	port_t owner_port,
	port_t task_port,
	vm_offset_t *vaddrp,
	vm_offset_t paddr,
	vm_size_t size )
{
	vm_map_t	task_map;
	kern_return_t	r;
	
	paddr = trunc_page( paddr );
	size = round_page( size );
	
	/* Port to map conversion increments the ref count on the map */
	if ( (r = NDPortToMap( task_port, &task_map )) != KERN_SUCCESS )
	{
printf("NDPortToMap fails %d\n", r );
		return r;
	}
	if ( (r = vm_allocate( task_map, vaddrp, size, TRUE )) != KERN_SUCCESS )
	{
printf("vm_allocate fails %d\n", r );
		vm_map_deallocate(task_map);	/* Decrement ref count. */
		return r;
	}
	if ( (r = ND_ioaccess( task_map, paddr, *vaddrp, size )) != KERN_SUCCESS )
	{
printf("ND_ioaccess fails %d\n", r );
		vm_map_deallocate(task_map);	/* Decrement ref count */
		return r;
	}
	vm_map_deallocate(task_map);		/* Decrement ref count */
	return KERN_SUCCESS;
}

/*
 * Make an IO device area accessible at physical address 'phys'
 * by mapping task ptes starting at 'virt'.  Note that entry
 * is marked non-cacheable.
 */
 static kern_return_t
ND_ioaccess (vm_map_t map, vm_offset_t phys, vm_offset_t virt, vm_size_t size)
{
	register vm_offset_t	pa = phys;
	register vm_offset_t	va = virt;
	register int		i = atop (round_page (size));
	register pmap_t		pmap = vm_map_pmap(map);

	if ( pmap == PMAP_NULL )
		return KERN_FAILURE;
	/*
	 * Enter each page into the pmap for the user's task.
	 */
	do {
		pmap_enter (pmap, va, pa, VM_PROT_READ | VM_PROT_WRITE, 0);
		pa += page_size;
		va += page_size;
	} while (--i > 0);

	pflush_user();		/* Flush the modified user PTEs to physical memory. */
	return KERN_SUCCESS;
}

/*
 * Convert the user task port into a kernel vm_map_t structure.
 */
 kern_return_t
NDPortToMap( port_t task_port, vm_map_t *map )
{
	kern_port_t	kport;
	extern vm_map_t	convert_port_to_map();

	if ( object_copyin(	current_task(),
				task_port,
				MSG_TYPE_PORT,
				FALSE,
				&kport ) == FALSE )
	{
		return(KERN_INVALID_ARGUMENT);
	}

	/* Convert_port_to_map() increments the ref cnt.  Remember to decrement! */
	if ( (*map = convert_port_to_map( kport )) == VM_MAP_NULL)
	{
		port_release( kport );
		return(KERN_INVALID_ARGUMENT);
	}
	port_release( kport ); /* Decrement ref count bumped in object_copyin() */
	return KERN_SUCCESS;
}

/*
 * Our flavor of map_addr.  This version doesn't panic when out of memory.
 * The only way that we get out of memory is through leaks and repeated loading.
 * That only happens in development. (And only when I add a bug, at that!)
 *
 * Strictly for use inside the loadable driver and kernel.
 */
caddr_t
ND_map_addr(register caddr_t addr, int size)
{
	register caddr_t reg;
	caddr_t ioaccess(vm_offset_t phys, vm_offset_t virt, vm_size_t size);

	/* setup mapping if outside transparently mapped space */
	if (addr < (caddr_t) P_EPROM ||
	    addr >= (caddr_t) (P_MAINMEM + P_MEMSIZE)) {
		if ((reg = (caddr_t) kmem_alloc_pageable (kernel_map, size))
		     == 0) {
			printf("ND_map_addr: no memory for device (0x%x: %d bytes)\n",
				addr, size );
			return 0;
		}
		reg = ioaccess ((vm_offset_t)addr, (vm_offset_t)reg,
			(vm_size_t)size);
		return(reg);
	}
	return(0);
}

 static kern_return_t
validate_access( port_t owner_port, int unit )
{
	ND_var_t *sp;
	
	if ( UNIT_INVALID(unit) )
		return KERN_INVALID_ARGUMENT;

	sp = &ND_var[UNIT_TO_INDEX(unit)];

	if ( ! sp->present )
		return KERN_INVALID_ADDRESS;
		
	if ( owner_port == PORT_NULL )
		return KERN_NO_ACCESS;
	if ( owner_port != sp->owner_port && owner_port != sp->debug_port )
		return KERN_NO_ACCESS;

	return KERN_SUCCESS;
}

