/*
 * Copyright 1990 NeXT, Inc.
 *
 * NextDimension kernel server.
 *
 * This server implements a Mach driver that monitors NextDimension devices for
 * interrupts and dispatches a message on receipt of an interrupt.
 */
#include "ND_patches.h"			/* Must be FIRST! */
#include <kernserv/kern_server_types.h>	/* Caution: imports mach_user_internal.h */
#include <sys/mig_errors.h>
#include <nextdev/slot.h>
#include <next/pcb.h>
#include <next/scr.h>
#include <next/bmap.h>
#include <vm/vm_kern.h>
#include <sys/callout.h>
#include "ND_var.h"

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

ND_var_t ND_var[SLOTCOUNT];

static void *ReplyBuf = (void *) 0;	/* Small buffer used for reply messages. */
/*
 * Instance variable used by the kern_server, defined in kernserv/kern_server_types.h.
 */
kern_server_t instance;	

/*
 * Initialize the argument for all port mappings used on the specified unit.
 *
 * Check for the presence of an NBIC on the CPU board.
 * Gain access to the NextBus, and check for the presence of boards
 * at each slot.  Note which slots have boards for future reference.
 *
 * Once we have finished boards, upgrade this to read the vendor and board
 * ID for the ND board before we mark it as present.
 */
void ND_init(void)
{
	extern int nbic_present; 	/* defined in nextdev/nbic.c */
	extern lock_t	lock_alloc();
	extern void 	*kalloc();
	int		slotid;
	int		s;
	port_t		port;

	ReplyBuf = kalloc( ND_MAX_REPLYSIZE );
	/*
	 * Initialize the ND_var array.
	 * Do this before testing for systems which can't
	 * possibly support a ND board, so that when PostScript
	 * asks us what slots have ND boards in them, we can
	 * truthfully reply with NONE!
	 */
	bzero( ND_var, sizeof ND_var );
	for ( slotid = 0; slotid < SLOTCOUNT; ++slotid )
	{
	    ND_var[slotid].present = FALSE;
	    ND_var[slotid].attached = FALSE;
	}	
	/* 
	 * HACK!  Patch our way past the BMAP chip.
	 */
	switch (machine_type) {
	    case NeXT_X15:
	    	/*
		 * BMAP chip: ROM must be unmapped.
		 */
		 bmap_chip->bm_lo = 0;
		/* Unmap the ROM overlay, allowing access to the bus */
		*scr2 |= SCR2_OVERLAY;
		 break;
		 
	    case NeXT_CUBE:
		/* Unmap the ROM overlay, allowing access to the bus */
		*scr2 |= SCR2_OVERLAY;
	    	break;
		
	    case NeXT_WARP9:
	    case NeXT_WARP9C:
	    	/* No NextBus on these systems */
	    	return;
	}
	/*
	 * Check for the presence of an NBIC on the host board.  If
	 * missing, give up.
	 */
	if ( ! nbic_present )
	    return;
	

	for ( slotid = 0; slotid < SLOTCOUNT; ++slotid )
	{
	    simple_lock_init( &ND_var[slotid].flags_lock );
	    ND_var[slotid].map_self = current_map();
	    ND_var[slotid].slot = slotid << 1;
	    ND_var[slotid].thread = current_thread();
	    ND_var[slotid].lockno = LOCK_NUM;
	    if ( slotid != 0 )	/* Don't map in the CPU board! */
		ND_var[slotid].slot_addr =
				ND_map_addr(ND_SLOT_PHYS_ADDR(slotid), ND_SLOT_SIZE);
	}

	/*
	 * Probe each slot, looking for an NBIC.  Mark as 
	 * present any slot which responds at an NBIC register address.
	 * Don't check CPU 0, as that is the host and not a ND board!
	 * Check board and vendor ID in finished version.
	 */
	for ( slotid = 1; slotid < SLOTCOUNT; ++slotid )
	{
	    if ( probe_rb( ND_mask_register(slotid) ) )
	    {
	        ND_var[slotid].present = TRUE;
		/* A board is present.  Map in the rest of the hardware. */
		ND_var[slotid].csr_addr =
				ND_map_addr(ND_CSR_PHYS_ADDR(slotid), ND_CSR_SIZE);
#if ! defined(PROTOTYPE)
		ND_var[slotid].bt_addr =
				ND_map_addr(ND_BT_PHYS_ADDR(slotid), ND_BT_SIZE);
#endif
#if ! defined(CURSOR_i860)
		ND_var[slotid].fb_addr =
				ND_map_addr(ND_FB_PHYS_ADDR(slotid), ND_FB_SIZE);
#endif
		ND_var[slotid].globals_addr =
			ND_map_addr(ND_GLOBALS_PHYS_ADDR(slotid), ND_GLOBALS_SIZE);
		/* Used to set mapping in service threads. */
		ND_var[slotid].board_addr = ND_BOARD_PHYS_ADDR(slotid);
		
		/*
		 * If a mapping failed, free all resources and pretend that the
		 * board isn't here.  It's all we can really do.
		 */
		if (   ND_var[slotid].csr_addr == (vm_offset_t) 0
#if ! defined(PROTOTYPE)
		    || ND_var[slotid].bt_addr == (vm_offset_t) 0
#endif
#if ! defined(CURSOR_i860)
		    || ND_var[slotid].fb_addr == (vm_offset_t) 0
#endif
		    || ND_var[slotid].globals_addr == (vm_offset_t) 0 )
		{
		    printf("NextDimension Slot %d: ND_map_addr map fails\n",slotid<<1);
		    ND_var[slotid].present = FALSE;
		    if ( ND_var[slotid].csr_addr != (vm_offset_t) 0 )
		    	kmem_free( kernel_map, ND_var[slotid].csr_addr, ND_CSR_SIZE );
#if ! defined(PROTOTYPE)
		    if ( ND_var[slotid].bt_addr != (vm_offset_t) 0 )
		    	kmem_free( kernel_map, ND_var[slotid].bt_addr, ND_BT_SIZE );
#endif
#if ! defined(CURSOR_i860)
		    if ( ND_var[slotid].fb_addr != (vm_offset_t) 0 )
		    	kmem_free( kernel_map, ND_var[slotid].fb_addr, ND_FB_SIZE );
#endif
		    if ( ND_var[slotid].globals_addr != (vm_offset_t) 0 )
		    	kmem_free(kernel_map, ND_var[slotid].globals_addr,
								 ND_GLOBALS_SIZE);
		    ND_var[slotid].csr_addr = (vm_offset_t) 0;
#if ! defined(PROTOTYPE)
		    ND_var[slotid].bt_addr = (vm_offset_t) 0;
#endif
		    ND_var[slotid].fb_addr = (vm_offset_t) 0;
		    ND_var[slotid].globals_addr = (vm_offset_t) 0;
		    ND_var[slotid].board_addr = (vm_offset_t) 0;
		    
		    if ( ND_var[slotid].slot_addr != (vm_offset_t) 0 )
			    kmem_free( kernel_map, ND_var[slotid].slot_addr,
			    					ND_SLOT_SIZE );
		    ND_var[slotid].slot_addr = (vm_offset_t) 0;
		    ND_var[slotid].present = FALSE;

		}
	    }
	    else	/* Free the mapped slot space page */
	    {
	        if ( ND_var[slotid].slot_addr != (vm_offset_t) 0 )
	    		kmem_free(kernel_map, ND_var[slotid].slot_addr, ND_SLOT_SIZE);
		ND_var[slotid].slot_addr = (vm_offset_t) 0;
	    }

	}

	/*
	 * Hook in the interrupt handler for this driver.
	 */
	install_polled_intr(I_REMOTE, ND_intr);
	
	/* Initialize each piece of hardware found. */
	for ( slotid = 0; slotid < SLOTCOUNT; ++slotid )
	{
	    if ( ND_var[slotid].present )
	    {
	    	ND_var[slotid].lock = lock_alloc();
		lock_init(ND_var[slotid].lock, TRUE);	/* It's a sleep lock */
		ND_init_hardware( &ND_var[slotid] );
		printf( "NextDimension board present in Slot %d\n", slotid << 1 );
	    }
	}
}

/*
 * Called when driver is being unloaded from the kernel, this code
 * disables interrupts from all NextBus devices we have attached.
 */
void ND_signoff(void)
{
	int i;
	for (i = 0; i < SLOTCOUNT; i++)
	{
	    if ( ND_var[i].attached == TRUE )
	    {
		ND_UnregisterThyself( &ND_var[i] );	/* Remove our screen from EV system */
		ND_stop_services( &ND_var[i] );
	    }
	    if ( ND_var[i].slot_addr != (vm_offset_t) 0 )
		kmem_free( kernel_map, ND_var[i].slot_addr, ND_SLOT_SIZE );
	    if ( ND_var[i].csr_addr != (vm_offset_t) 0 )
		kmem_free( kernel_map, ND_var[i].csr_addr, ND_CSR_SIZE );
#if ! defined(PROTOTYPE)
	    if ( ND_var[i].bt_addr != (vm_offset_t) 0 )
		kmem_free( kernel_map, ND_var[i].bt_addr, ND_BT_SIZE );
#endif
#if ! defined(CURSOR_i860)
	    if ( ND_var[i].fb_addr != (vm_offset_t) 0 )
		kmem_free(kernel_map, ND_var[i].fb_addr, ND_FB_SIZE);
#endif
	    if ( ND_var[i].globals_addr != (vm_offset_t) 0 )
		kmem_free(kernel_map, ND_var[i].globals_addr, ND_GLOBALS_SIZE);
	    if ( ND_var[i].present == TRUE )
	    {
		if ( ND_var[i].lock != (lock_t) 0 )
			lock_free(ND_var[i].lock);
	    }
	}
	uninstall_polled_intr(I_REMOTE, ND_intr);
	if ( ReplyBuf != (void *) 0 )
		kfree( ReplyBuf, (long)ND_MAX_REPLYSIZE );
}

/*
 * Called when a port dies, if this is one of our owner ports close
 * out the ownership and interrupt attachment and return TRUE.  Otherwise,
 * return FALSE.
 */
boolean_t ND_port_death(port_name_t port)
{
	int i;
	boolean_t retval = FALSE;
	for (i = 0; i < SLOTCOUNT; i++) {
		ND_var_t *sp = ND_var+i;
		if (sp->owner_port == port) {
			ND_stop_services( sp );
			sp->attached = FALSE;	/* Owner gone.  Detach the unit */
			sp->owner_port = PORT_NULL;
			sp->negotiation_port = PORT_NULL;
			retval = TRUE;
			continue;
		} else if (sp->negotiation_port == port) {
			sp->negotiation_port = PORT_NULL;
			retval = TRUE;
			continue;
		} else if (sp->debug_port == port) {
			sp->debug_port = PORT_NULL;
			if ( sp->present )
			    ND_update_message_system( sp );
			retval = TRUE;
			continue;  /* The same port may be published to all slots */
		} else
			continue;
	}
	return retval;
}


/*
 * ND_msg:
 *
 *	called from kern_server dispatch loop.
 *
 *	Take the message, craft a reply buffer, and pass the message and
 *	reply buffer to the MIG generated ND_server().  If a reply is expected,
 *	send it out.
 */
kern_return_t ND_msg (
	msg_header_t *msg,
	int	arg)
{
	typedef struct {
		msg_header_t Head;
		msg_type_t RetCodeType;
		kern_return_t RetCode;
	} Reply;
	Reply *out_msg;
	kern_return_t ret_code;

	out_msg = (Reply *)ReplyBuf;
	
	/*
	 * Call the MIG generated interface.  This in turn will call code over in
	 * ND_services.c to execute the desired functions.
	 */
	ND_server(msg, (msg_header_t *)out_msg);
	
	if (out_msg->RetCode == MIG_NO_REPLY)
	    ret_code = KERN_SUCCESS;
	else
	{
	    if ( out_msg->Head.msg_size > ND_MAX_REPLYSIZE )
	    	printf( "NextDimension: Msg ID %d reply too large!\n",
				(out_msg->Head.msg_id - 100) );
	    ret_code = msg_send( (msg_header_t *) out_msg, MSG_OPTION_NONE, 0 );
	}
			
	return ret_code;
}

/*
 *	These routines are located in this module to take advantage of the external
 *	interface available here.
 */
 kern_return_t
ND_allocate_resources( ND_var_t *sp )
{
	kern_return_t r;
	task_t task = task_self();
	
	if ( (r = port_set_allocate(task, &sp->ND_portset)) != KERN_SUCCESS )
		return r;

	if ( (r = port_allocate( task, &sp->kernel_port )) != KERN_SUCCESS )
		return r;
	
	if ( (r = port_allocate( task, &sp->service_port )) != KERN_SUCCESS )
		return r;

	if ( (r = port_allocate(task, &sp->service_replyport)) != KERN_SUCCESS )
		return r;

	/* Set up our listener port set, with it's initial port */
	if ( (r = port_set_add(task,sp->ND_portset,sp->kernel_port)) != KERN_SUCCESS )
		return r;

	/* Publish the name of the initial port */
	sp->portnames = (ND_port_name *)kalloc( sizeof (ND_port_name) );
	sp->portnames->port = sp->kernel_port;
	sp->portnames->name = (char *)kalloc( strlen("kernel") + 1 );
	strcpy( sp->portnames->name, "kernel" );
	sp->portnames->next = (ND_port_name *)0;

	/* This task acts as the backing store for the i860 VM system. */
	if ( (r = task_create( task, FALSE, &sp->pager_task )) != KERN_SUCCESS )
		return r;
	r = NDPortToMap( (port_t)sp->pager_task, &sp->map_pager );
	if ( r != KERN_SUCCESS )
		return r;
	return KERN_SUCCESS;
}

 void
ND_deallocate_resources( ND_var_t *sp )
{
	ND_port_name *p;
	ND_port_name *np;
	kern_return_t r;
	task_t task = task_self();

	/* Destroy the portset, incidentally removing all ports from the set. */
	(void) port_set_deallocate( task, sp->ND_portset );
	
	/* Free all allocated ports, and associated storage. */
	(void) port_deallocate( task, sp->service_port );
	(void) port_deallocate( task, sp->service_replyport );

	lock_write(sp->lock);	/* Other threads are still running... */
	p = sp->portnames;
	while ( p != (ND_port_name *)0 )
	{
		np = p->next;
		if ( p->name )
			kfree( p->name, strlen( p->name ) + 1 );
		(void) port_deallocate( task, p->port );
		kfree( p, sizeof (ND_port_name) );
		p = np;
	} 
	sp->portnames = (ND_port_name *)0;
	lock_done(sp->lock);
	
	/* Destroy the pager backing store and derived objects. */
	ND_Kern_destroy_pagecache( sp );
	if ( sp->map_pager != (vm_map_t) 0 )
		vm_map_deallocate(sp->map_pager);	/* Decrement ref count */
	
	r = task_terminate( sp->pager_task );
	if ( r != KERN_SUCCESS )
		printf( "Pager task terminate fails: %d\n", r );
}

/*
 *	ND_Kern port management functions.  These are called from the i860 via a MiG 
 *	generated interface.
 */
 
 kern_return_t
ND_Kern_port_allocate( port_t serv_port, port_t *port_ptr )
{
	ND_port_name *p;
	task_t task = task_self();
	ND_var_t *sp = (ND_var_t *)serv_port;
	kern_return_t r;

	if ( (r = port_allocate( task, port_ptr )) != KERN_SUCCESS )
		return r;
	if ( (r = port_set_add( task, sp->ND_portset, *port_ptr )) != KERN_SUCCESS )
		return r;

	p = (ND_port_name *)kalloc( sizeof (ND_port_name) );
	p->port = *port_ptr;
	p->name = (char *)0;

	lock_write(sp->lock);
	p->next = sp->portnames;	/* Head insert on our port list */
	sp->portnames = p;
	lock_done(sp->lock);
	return KERN_SUCCESS;
}

 
 kern_return_t
ND_Kern_port_deallocate( port_t serv_port, port_t port )
{
	ND_port_name *p;
	ND_port_name *pprev;
	task_t task = task_self();
	ND_var_t *sp = (ND_var_t *)serv_port;
	
	lock_read(sp->lock);
	p = sp->portnames;
	while ( p != (ND_port_name *) 0 )
	{
		if ( p->port == port )
			break;
		pprev = p;
		p = p->next;
	}
	lock_done(sp->lock);
	if ( p == (ND_port_name *) 0 )
		return KERN_INVALID_ARGUMENT;	/* No such port! */

	/* Pull entry out of list. */
	lock_write(sp->lock);
	if ( p == sp->portnames )
		sp->portnames = p->next;
	else
		pprev->next = p->next;
	lock_done(sp->lock);

	/* Free resources. */
	if ( p->name )
		kfree( p->name, strlen( p->name ) + 1 );
	(void) port_set_remove( task, p->port );
	(void) port_deallocate( task, p->port );
	kfree( p, sizeof (ND_port_name) );
	return KERN_SUCCESS;
}

/*
 * Wraps for wiring and unwiring ranges of memory.
 *
 * These let us avoid including kernserv/kern_server_types.h in
 * other modules, with all the bad side effects that would have.
 */
 kern_return_t
ND_wire_range( vm_address_t addr, vm_size_t size )
{
	return kern_serv_wire_range( (void *)&instance, addr, size );
}

 kern_return_t
ND_unwire_range( vm_address_t addr, vm_size_t size )
{
	return kern_serv_unwire_range( (void *)&instance, addr, size );
}
