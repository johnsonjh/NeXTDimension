#include <kern/mach_types.h>
#include "i860/proc.h"
#include "i860/vm_types.h"
#include "vm/vm_param.h"
#include "vm/vm_kern.h"
#include "ND/NDmsg.h"
#include "ND/NDreg.h"
#include "ND/ND_types.h"
#include <sys/port.h>
#include <sys/kern_return.h>

#define ServicePort	(MSG_QUEUES->service_port)
#define ReplyPort	(MSG_QUEUES->service_replyport)

extern  kern_return_t pmap_translation_valid( unsigned long, vm_address_t, vm_prot_t );
extern	pt_entry_t *pmap_pte(unsigned long, vm_offset_t);

/*
 * Virtual Memory demand page in:
 *
 */
 kern_return_t
vm_fault(	unsigned long dirbase,
		vm_address_t vaddr,
		vm_prot_t fault_type,
		boolean_t change_wiring )
{
	kern_return_t r;
	volatile pt_entry_t *pte;
	vm_movepage_t	pagein;
	vm_offset_t page;
	int s;
	
	/* Is the access to a valid address, in a valid mode? */
	if ((r = pmap_translation_valid( dirbase, vaddr, fault_type )) != KERN_SUCCESS)
		return r;
	if ( (pte = pmap_pte( dirbase, vaddr )) == PT_ENTRY_NULL )
		return KERN_INVALID_ADDRESS;

	/* Is the desired virtual address busy?  If so, wait for it to be available. */
	s = splhigh();
	while ( pte->wired && ! pte->valid )
		Sleep( trunc_page(vaddr), CurrentPriority() );
	/* Is the page now resident?  If so, we are done! */
	if ( pte->valid ) {
		splx(s);
		return KERN_SUCCESS;
	}
	/*
	 * Mark the virtual address/page as busy, so some other thread doesn't
	 * fault on it and start it paging in.
	 */
	pte->wired = 1;    /* State is now wired & !valid, indicating a pager lock */
	splx(s);
	/* Set up the page in memory. */
	if ((page = (vm_address_t)kmem_alloc(kernel_map, PAGE_SIZE)) == 0)
		panic( "vm_fault: out of memory." );

	page = kvtophys(page);
	/* Load the page from the backing store, or zero-fill as appropriate. */
	if ( pte->backing ) {
		pagein.vaddr = (vm_address_t)trunc_page(vaddr);
		pagein.paddr = (vm_address_t)page;
		pagein.size = PAGE_SIZE;
		r = ND_Kern_page_in( ServicePort, ReplyPort, &pagein, 1 );
		if ( r != KERN_SUCCESS ) {
			printf( "ND_Kern_page_in 0x%X to 0x%X returns %D\n",
				trunc_page(vaddr), page, r );
			panic( "ND_Kern_page_in" );
		}
	}

	/*
	 * If the page was allocated as writable, make sure we preserve write
	 * permission on it.
	 */
	if ( (pte->prot & i860_KRW) != 0 )
		fault_type |= VM_PROT_WRITE;
	/*
	 * Clear the busy state on the page, and wake up anyone waiting for the page
	 * to be available.
	 */
	pmap_enter( dirbase, vaddr, page, fault_type, change_wiring );
	Wakeup( trunc_page(vaddr) );
	return KERN_SUCCESS;
}