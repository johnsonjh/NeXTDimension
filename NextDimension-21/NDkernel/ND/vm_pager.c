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
 * Virtual Memory flush:
 *
 *	Flush all dirty pages in a range of addresses.
 */
 kern_return_t
vm_flush(	task_t task,
		vm_address_t vaddr,
		vm_size_t size )
{
	pmap_clean(current_pmap(), trunc_page(vaddr), round_page(vaddr+size));
	
	return KERN_SUCCESS;
}

/*
 * Virtual Memory page out:
 *
 *	Flush all dirty pages in a range of addresses.
 *	Release the physical memory behind the pages,
 *	marking their state as paged out.
 */
 kern_return_t
vm_page_out(	task_t task,
		vm_address_t vaddr,
		vm_size_t size )
{
	unsigned long pmap = current_pmap();
	
	pmap_clean(pmap, trunc_page(vaddr), round_page(vaddr+size));
	pmap_release_range(pmap, trunc_page(vaddr), round_page(vaddr+size));
	
	return KERN_SUCCESS;
}
