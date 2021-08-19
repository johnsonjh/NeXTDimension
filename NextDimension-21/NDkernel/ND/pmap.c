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

#define START_MANAGED_MEM	(_slot_id_ | (((unsigned)ADDR_DRAM)&0x0FFFFFFF))
#define END_MANAGED_MEM		(START_MANAGED_MEM + PADDR_DRAM_SIZE)
#define ServicePort	(MSG_QUEUES->service_port)
#define ReplyPort	(MSG_QUEUES->service_replyport)

/* Forward declarations */
void pmap_remove_range();

boolean_t cache_on_or_off = 0;	/* 1 disables cacheing, 0 enables the cache */
int	ptes_per_vm_page;	/* PTEs needed to map one VM page */

extern unsigned long FreePageCnt;
extern unsigned long MinFreePages;

#define DEBUG 0

/*
 *	Assume that there are three protection codes, all using low bits.
 */
int	kernel_protection_codes[8];

pmap_init()
{
	i860_protection_init();
	/*
	 *	Set ptes_per_vm_page for general use.
	 */
	ptes_per_vm_page = page_size / i860_PGBYTES;

}
i860_protection_init()
{
	register int	*kp, *up, prot;

	kp = kernel_protection_codes;
	for (prot = 0; prot < 8; prot++) {
		switch (prot) {
		case VM_PROT_NONE | VM_PROT_NONE | VM_PROT_NONE:
			*kp++ = i860_NO_ACCESS;
			break;
		case VM_PROT_READ | VM_PROT_NONE | VM_PROT_NONE:
		case VM_PROT_READ | VM_PROT_NONE | VM_PROT_EXECUTE:
		case VM_PROT_NONE | VM_PROT_NONE | VM_PROT_EXECUTE:
			*kp++ = i860_KR;
			break;
		case VM_PROT_NONE | VM_PROT_WRITE | VM_PROT_NONE:
		case VM_PROT_NONE | VM_PROT_WRITE | VM_PROT_EXECUTE:
		case VM_PROT_READ | VM_PROT_WRITE | VM_PROT_NONE:
		case VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXECUTE:
			*kp++ = i860_KRW;
			break;
		}
	}
}

/*
 *	Given a machine independent protection code,
 *	convert to a i860 protection code.
 */

#define	i860_protection(prot)	(kernel_protection_codes[prot])

/*
 *	Given an offset and a dirbase, compute the address of the
 *	pte.  If the address is invalid with respect to the dirbase
 *	(e.g., we need to grow the pmap) then PT_ENTRY_NULL is returned.
 *
 *	This is only used internally.
 */
pt_entry_t *pmap_pte(dirbase, addr)
	register unsigned long	dirbase;
	register vm_offset_t	addr;
{
	register pt_entry_t	*ptp;
	register pt_entry_t	pte;
	register pt_entry_t	*pde;

	if (dirbase == 0)
		return(PT_ENTRY_NULL);
		
	pde = (pt_entry_t *)phystokv((dirbase & i860_PTE_PFN));
	
	pte = pde[pdenum(addr)];
	if (pte.valid == 0)
		return(PT_ENTRY_NULL);
	ptp = (pt_entry_t *)ptetokv(pte);
	return(&ptp[ptenum(addr)]);
}

/*
 *	kvtophys(addr)
 *
 *	Convert a kernel virtual address to a physical address
 */
kvtophys(addr)
vm_offset_t addr;
{
	pt_entry_t *pte;

	if ((pte = pmap_pte( current_pmap(), addr)) == PT_ENTRY_NULL)
		return(0);
	return(i860_trunc_page(*((int *)pte)) | (addr & i860_OFFMASK));
}

/*
 * Test address for validity.
 * Check desired access permissions against what we actually have.
 * The access checks are weak.  In supervisor mode (the way we run everything)
 * we have read/execute access to everything.  All we can control is write permission.
 */
 kern_return_t
pmap_translation_valid( unsigned long dirbase,
			vm_address_t vaddr,
			vm_prot_t access)
{
	register pt_entry_t	*pte;
	if ( (pte = pmap_pte(dirbase, vaddr)) == PT_ENTRY_NULL )
		return KERN_INVALID_ADDRESS;
	if ( ! pte->allocated )
		return KERN_INVALID_ADDRESS;
	
	/* There's a valid translation. See if the permissions are OK. */
	if ( (access & VM_PROT_WRITE) && (pte->prot & i860_KRW) == 0 )
		return KERN_PROTECTION_FAILURE;

	return KERN_SUCCESS;
}

/*
 *	Insert the given physical page (p) at
 *	the specified virtual address (v) in the
 *	target physical map with the protection requested.
 *
 *	If specified, the page will be wired down, meaning
 *	that the related pte can not be reclaimed.
 *	This code assumes that the virtual address has already
 *	been allocated.
 *
 *	NB:  This is the only routine which MAY NOT lazy-evaluate
 *	or lose information.  That is, this routine must actually
 *	insert this page into the given map NOW.
 */
void pmap_enter(pmap, v, pa, prot, wired)
	register unsigned long	pmap;
	vm_offset_t		v;
	register vm_offset_t	pa;
	vm_prot_t		prot;
	boolean_t		wired;
{
	register pt_entry_t	*pte;
	register int		i;
	pt_entry_t		template;
	int			vp;
	vm_offset_t		old_pa;

#if DEBUG > 1
	printf("pmap(%x, %x)\n", v, pa);
#endif
	if ( pmap == 0 )
		return;

	vp = i860_protection(prot); 	/* Bits for the desired protection. */
	/*
	 *	Expand pmap to include this pte.  Assume that
	 *	pmap is always expanded to include enough i860
	 *	pages to map one VM page.
	 */

	while ((pte = pmap_pte(pmap, v)) == PT_ENTRY_NULL) {
		pmap_expand(pmap, v);	/* Shouldn't happen with allocated pages. */
	}
	/*
	 *	Special case if the physical page is already mapped
	 *	at this address.
	 */
	old_pa = i860_ptob(pte->pfn);
	if (old_pa == pa) {
	    /*
	     *	May be changing its wired attribute or protection
	     */
	    invalidate_TLB();
	    i = ptes_per_vm_page;
	    do {
	        template = *pte;
		template.cache = cache_on_or_off;
		template.prot = vp;
		template.wired = (wired != 0);
		*pte = template;
		INCR_PT_ENTRY(pte);
	    } while (--i > 0);
	    invalidate_TLB();	/* Didn't change PFN, so don't need a full flush */
	}
	else {

	    /*
	     *	Remove old mapping from the PV list if necessary.
	     */
	    if (old_pa != (vm_offset_t) 0) {
		/*
		 *	Invalidate the translation buffer,
		 *	then remove the mapping.
		 */
		invalidate_TLB();
		/*
		 *	Don't free the pte page if removing last
		 *	mapping - we will immediately replace it.
		 */
		pmap_remove_range(pmap, v, v + i860_ptob(ptes_per_vm_page));
	    }

	    /*
	     *	Build a template to speed up entering -
	     *	only the pfn changes.
	     *
	     *  Cache and backing store class are preserved from the
	     *  original PTE settings.  If the PTE is new, it's in the 
	     *  default cache mode and has a zero-fill backing store.
	     *
	     */
	    template = *pte;
	    template.valid = 1;
	    template.allocated = 1;
	    template.backing = pte->backing;	/* Initially zero-fill */
	    template.prot = vp;
	    template.pfn = i860_btop(pa);
	    if (wired)
	    	template.wired = 1;
	    else
	    	template.wired = 0;
	    /*
	     * If the physical address is outside the range we manage, it's
	     * probably hardware.  Mark these pages as uncached, dirty, and
	     * accessed, so we won't fault on them for maintenance reasons.
	     */
	    if (pa < START_MANAGED_MEM || pa >= END_MANAGED_MEM) {
		/*
		 *	Outside range of managed physical memory.
		 *	Set up as mapped hardware.
		 */
		 template.modify = 1;	/* Dirty and accessed, so we won't trap */
		 template.ref = 1;
		 template.cache = 1;	/* Disable cache! */
	    }
	    i = ptes_per_vm_page;
	    do {
	    	*pte = template;
		INCR_PT_ENTRY(pte);
		template.pfn++;
	    } while (--i > 0);
	    flush_inv();
	}
}

/*
 * Grow the 2nd level map to cover the requested virt address range.
 */
/*
 *	Routine:	pmap_expand
 *
 *	Expands a pmap to be able to map the specified virtual address.
 *
 *	Allocate a new block of 2nd level PTEs.
 *	Check to see if another thread may have already loaded the page table entry.
 *	If so, free the block and return.
 *
 *	Make sure that the PTE for the new block (All phys memory is mapped!)
 *	has the cache disabled.  The i860 translation mechanism won't look in the 
 *	data cache for pages.
 *
 *	Add the new block of PTEs to the page directory.
 *	
 */
pmap_expand(map, v)
	register unsigned long	map;
	register vm_offset_t	v;
{
	pt_entry_t		*ptep, *pdp, *pte;
	int			i;

	/*
	 *	Allocate a VM page for the level 2 page table entries
	 * 	Since kmem_alloc() may sleep, we check after the allocation
	 *	to see if someone else set up the 2nd level entry, and possibly
	 *	discard the allocated page.
	 */
	if ((ptep = (pt_entry_t*)kmem_alloc(kernel_map, i860_PGBYTES))==PT_ENTRY_NULL)
		return;
	/*
	 *	See if someone else expanded us first
	 */
	if (pmap_pte(map, v) != PT_ENTRY_NULL) {
		kmem_free(kernel_map, ptep, i860_PGBYTES);
		return;
	}
	/*
	 * Make sure that this new page isn't cacheable.  The i860 really
	 * doesn't like having PTEs stuck in the data cache...
	 */
	if ( (pte = pmap_pte(map, (vm_offset_t) ptep)) == PT_ENTRY_NULL )
		printf( "pmap_expand: kmem_alloc gave us a bogus page!\n");
	i = ptes_per_vm_page;
	do {
	    pte->cache = 1;	/* Set the cache DISABLE bit. */
	    INCR_PT_ENTRY(pte);
	} while (--i > 0);
	/*
	 *	Set the page directory entry for this page table entry.
	 */
	pdp = (pt_entry_t *) phystokv(map);
	pdp = &pdp[pdenum(v)];
	*((int *)pdp) = 0;	/* clear the entry, then fill in details. */
	pdp->pfn = i860_btop(kvtophys(ptep));
	pdp->prot = i860_UW;
	pdp->valid = 1;
	/* We changed a PDE and a page frame address, so a full flush is required... */
	flush_inv();	/* And update the TLB */
	return;
}
/*
 *	Remove a range of i860 page-table entries.
 *	The entries given are the first (inclusive)
 *	and last (exclusive) entries for the VM pages.
 *	The virtual address is the va for the first pte.
 *
 *	The pmap must be locked.
 *	If the pmap is not the kernel pmap, the range must lie
 *	entirely within one pte-page.  This is NOT checked.
 *	Assumes that the pte-page exists.
 */

/* static */
void pmap_remove_range(pmap, va, e)
	unsigned long		pmap;
	vm_offset_t		va;
	vm_offset_t		e;
{
	register pt_entry_t	*cpte, *lpte;
	int			pfn;
	int			num_removed, num_unwired;
	register int		i;
	int			pai;
	vm_offset_t		pa, va_lvl2_align;

	num_removed = 0;
	num_unwired = 0;

	for (; va < e; va += i860_PGBYTES) {

		if ((cpte = pmap_pte(pmap, va)) == PT_ENTRY_NULL) {
			/*
			 *  Can only get here if the level 2 page
			 *  is missing.
			 *  va_lvl2_align is the amount of virtual
			 *  space mapped by a level 2 vm page.
			 */
			va_lvl2_align = (vm_offset_t)i860_ptob(NPTES);
			va &= ~(va_lvl2_align - 1);
			va += va_lvl2_align;
			/* Look out for wraparound near 2^32 */
			if ( va == 0 || va >= e )
				break;
			va -= i860_PGBYTES;
			continue;
		}
		if ( va == 0 )	/* Look out for wraparound near 2^32 */
			break;

		pfn = cpte->pfn;
		if (pfn != 0)
			num_removed++;
		if (cpte->wired)
			num_unwired++;

		*((int *)cpte) = 0;			/* Zap the PTE */
		if ( pfn == 0 )
			continue;	/* Don't free pages that aren't there */

		pa = i860_ptob(pfn);
		if (pa < START_MANAGED_MEM || pa >= END_MANAGED_MEM) {
			/*
			 *	Outside range of managed physical memory.
			 */
			continue;
		}
		pa = phystokv(pa);
		kmem_free( kernel_map, i860_PGBYTES, pa );	/* Free the page */
	}
	flush_inv();	/* flush, because we changed a PFN, and invalidate the TLB */

	return;
}

/*
 *	Remove the given range of addresses
 *	from the specified map.
 *
 *	It is assumed that the start and end are properly
 *	rounded to the i860 page size.
 */

void pmap_remove(map, s, e)
	unsigned long	map;
	vm_offset_t	s, e;
{
	int			spl;

	if (map == 0)
		return;

	/*
	 *	Invalidate the translation buffer first,
	 *	flushing any data to shared pages (Cache lines are
	 *	maintained by VIRTUAL address!)
	 */
	flush_inv();

	pmap_remove_range(map, s, e);

}

/*
 *	Release a range of i860 page-table entries.
 *	The entries given are the first (inclusive)
 *	and last (exclusive) entries for the VM pages.
 *	The virtual address is the va for the first pte.
 *
 *	The pmap must be locked.
 *	All dirty pages must have been cleaned.
 *	Wired pages are left alone.
 */

/* static */
void pmap_release_range(pmap, va, e)
	unsigned long		pmap;
	vm_offset_t		va;
	vm_offset_t		e;
{
	register pt_entry_t	*cpte, *lpte;
	vm_offset_t		pa, va_lvl2_align;

	va_lvl2_align = (vm_offset_t)i860_ptob(NPTES);

	for (; va < e; va += i860_PGBYTES) {

		if ((cpte = pmap_pte(pmap, va)) == PT_ENTRY_NULL) {
			/*
			 *  Can only get here if the level 2 page
			 *  is missing.
			 *  va_lvl2_align is the amount of virtual
			 *  space mapped by a level 2 vm page.
			 */
			va &= ~(va_lvl2_align - 1);
			va += va_lvl2_align;
			/* Look out for wraparound near 2^32 */
			if ( va == 0 || va >= e )
				break;
			va -= i860_PGBYTES;
			continue;
		}
		if ( va == 0 )	/* Look out for wraparound near 2^32 */
			break;

		/* If the page is not present, dirty, or wired, skip it. */
		if ( cpte->wired || cpte->modify || cpte->valid == 0 )
			continue;
		/*
		 * If the physical address of the page isn't part of
		 * our pool of managed memory, don't free it.
		 * (But it should be marked WIRED!)
		 */
		pa = i860_ptob(cpte->pfn);
		if (pa < START_MANAGED_MEM || pa >= END_MANAGED_MEM)
		{
			continue;
		}
		/* Free the page. */
		cpte->valid = 0;
		cpte->ref = 0;
		cpte->pfn = 0;
		pa = phystokv(pa);
		kmem_free( kernel_map, i860_PGBYTES, pa );
	}
	flush_inv();/* flush and invalidate TLB because we cleared VALID bit */

	return;
}

/*
 *	Release the given range of addresses
 *	from the specified map.  All pages in that range
 *	must have been cleaned already.  Wired pages are left alone.
 *
 *	It is assumed that the start and end are properly
 *	rounded to the i860 page size.
 */

void pmap_release(map, s, e)
	unsigned long	map;
	vm_offset_t	s, e;
{
	int			spl;

	if (map == 0)
		return;

	/*
	 *	Invalidate the translation buffer first,
	 *	flushing any data to shared pages (Cache lines are
	 *	maintained by VIRTUAL address!)
	 */
	flush_inv();

	pmap_release_range(map, s, e);

}


/*
 *	Add a range of i860 page-table entries.
 *	The entries given are the first (inclusive)
 *	and last (exclusive) entries for the VM pages.
 *	The virtual address is the va for the first pte.
 *
 *	By default, the entries are all marked for zero-fill.
 *
 *	The pmap must be locked.	(not a problem yet)
 */

/* static */
void pmap_add_range(pmap, va, e, prot, backing)
	unsigned long		pmap;
	vm_offset_t		va;
	vm_offset_t		e;
	vm_prot_t		prot;
	boolean_t		backing;
{
	register pt_entry_t	*cpte;
	pt_entry_t		template;

	*((int *)&template) = 0;
	/* Do we need to set the modify and ref bits too?  There was a chip bug... */
	template.allocated = 1;
	template.backing = backing;
	template.cache = cache_on_or_off;
	template.prot = i860_protection(prot);
	
	for (; va < e; va += i860_PGBYTES) {
	    if ( va == 0 )	/* Look out for wraparound near 2^32 */
		break;
	    /* Fetch/create the PTE for the page. */
	    while ((cpte = pmap_pte(pmap, va)) == PT_ENTRY_NULL)
		pmap_expand( pmap, va );

	    if ( cpte->allocated )	/* Ignore entries already in use. */
	    	continue;
	    *cpte = template;
	}
	invalidate_TLB();	/* And invalidate the TLB */

	return;
}

/*
 *	Add the given range of addresses
 *	to the specified map.
 *
 *	It is assumed that the start and end are properly
 *	rounded to the i860 page size.
 */

void pmap_add(map, s, e, prot, backing)
	unsigned long	map;
	vm_offset_t	s, e;
	vm_prot_t	prot;
	boolean_t	backing;
{
	if (map == 0)
		return;

	pmap_add_range(map, s, e, prot, backing);
}

/*
 * pmap_clean():
 *
 *	Sweep through a range of virtual addresses, looking for pages
 *	with the dirty bit set.  Write these dirty pages back out to the
 *	backing store, and clear the dirty bits on the affected pages.
 *
 *	Pages in transit are specially marked to avoid possible collisions.
 */
 void
pmap_clean( unsigned long pmap, vm_offset_t start, vm_offset_t end )
{
	register pt_entry_t	*pte;
	unsigned int		pfn;
	register vm_offset_t	va, pa;
	vm_offset_t		va_lvl2_align;
	vm_movepage_t		pageout[16];
	unsigned char		wirebits[16];
	int			pagecnt = 0;
	int			i;
	kern_return_t		r;

#if DEBUG
	printf("pmap_clean(%x, %x)\n", start, end);
#endif

	if ( start >= VM_MAX_ADDRESS )
		return;
	if ( end > VM_MAX_ADDRESS )
		end = VM_MAX_ADDRESS;

	va_lvl2_align = (vm_offset_t)i860_ptob(NPTES);
		
	flush();	/* Force all data out to DRAM */
	
	for ( va = start; va < end; va += i860_PGBYTES)
	{
		if ((pte = pmap_pte(pmap, va)) == PT_ENTRY_NULL)
		{
			/*
			 *  Can only get here if the level 2 page
			 *  is missing.
			 *  va_lvl2_align is the amount of virtual
			 *  space mapped by a level 2 vm page.
			 */
			va &= ~(va_lvl2_align - 1);
			va += va_lvl2_align;
			/* Look out for wraparound near 2^32 */
			if ( va == 0 || va >= end )
				break;
			va -= i860_PGBYTES;
			continue;
		}
		if ( va == 0 )	/* Look out for wraparound near 2^32 */
			break;
		/* We are only interested in valid dirty pages */
		if ( ! (pte->modify && pte->valid) )
			continue;
		/* It's dirty.  Make sure that it's also DRAM! */
		pfn = pte->pfn;
		pa = i860_ptob(pfn);
		if (pa < START_MANAGED_MEM || pa >= END_MANAGED_MEM) {
			/*
			 *	Outside range of managed physical memory.
			 */
			continue;
		}
		/* It's real.  Set it up to be flushed to the backing store. */
#if DEBUG
	printf("pmap_clean: dirty page %x, %x\n", va, pa);
#endif
		pageout[pagecnt].vaddr = va;
		pageout[pagecnt].paddr = pa;
		pageout[pagecnt].size = i860_PGBYTES;
		/* Save the old wired state */
		wirebits[pagecnt] = pte->wired;
		/* Set state to wired and invalid, marking page 'in transit' */
		pte->wired = 1;
		pte->valid = 0;
		/* Full batch? Send to the host, and update page states. */
		if ( ++pagecnt == 16 )
		{
#if DEBUG
	printf("pmap_clean: calling ND_Kern_page_out\n");
#endif
		    r = ND_Kern_page_out( ServicePort, ReplyPort,
		    			pageout, pagecnt );
		    if ( r != KERN_SUCCESS ) {
			    printf( "ND_Kern_page_out returns %D\n", r );
			    panic( "ND_Kern_page_put" );
		    }
		    pagecnt = 0;
		    for ( i = 0; i < 16; ++i )
		    {
		    	pte = pmap_pte(pmap, pageout[i].vaddr);
			pte->modify = 0;	/* Page cleaned */
			pte->backing = 1;	/* Page in from backing store*/
			pte->wired = wirebits[i]; /* Unlock page */	
			pte->valid = 1;
			Wakeup( va );	/* In case vm_fault() is waiting */
		    }
		    /* Other tasks may have run. Bring DRAM up to date. */
		    flush_inv();
		}
	}
	/* Any remaining pages? Send to the host, and update page states. */
	if ( pagecnt )
	{
#if DEBUG
	printf("pmap_clean: calling ND_Kern_page_out\n");
#endif
	    r = ND_Kern_page_out( ServicePort, ReplyPort,
				pageout, pagecnt );
	    if ( r != KERN_SUCCESS ) {
		    printf( "ND_Kern_page_out returns %D\n", r );
		    panic( "ND_Kern_page_put" );
	    }
	    for ( i = 0; i < pagecnt; ++i )
	    {
		pte = pmap_pte(pmap, pageout[i].vaddr);
		pte->modify = 0;		/* Page cleaned */
		pte->backing = 1;		/* Page in from backing store*/
		pte->wired = wirebits[i];	/* Unlock page */	
		pte->valid = 1;
		Wakeup( va );	/* In case vm_fault() is waiting */
	    }
	    flush_inv(); /* flush and invalidate the TLB */
	}

	return;
}

/*
 * Switch cache on or off for all user addresses.  A flag of 0 
 * disables cacheing, and a flag of 1 enables it.
 */
pmap_cache( unsigned long pmap, int flag )
{
	if (cache_on_or_off == flag)
		return;
	cache_on_or_off = ( flag ? 0 : 1);
	pmap_cache_range( pmap, flag, (vm_offset_t)0, (vm_offset_t) VM_MIN_KERNEL_ADDRESS );
}

pmap_cache_range(unsigned long pmap,int flag,vm_offset_t start,vm_offset_t end)
{
	register vm_offset_t	va, va_lvl2_align;
	register pt_entry_t	*ptep;

	for (va = start; va < end; va += i860_PGBYTES) {
		if ((ptep = pmap_pte(pmap, va)) == 0) {
			/*
			 *  Can only get here if the level 2 page
			 *  is missing.  Bump va up to the next
			 *  possible level 2 page block.
			 *  va_lvl2_align is the amount of virtual
			 *  space mapped by a level 2 vm page.
			 */
			va_lvl2_align = (vm_offset_t)i860_ptob(NPTES);
			va &= ~(va_lvl2_align - 1);
			va += va_lvl2_align;
			va -= i860_PGBYTES;
			continue;
		}
		if ( ptep->allocated )
			ptep->cache = (flag ? 0 : 1);	// A 1 DISABLES the cache!
	}
	flush_inv();
	return;
}



/*
 * PageOutDaemon():
 *
 *	This is a bogus single-task pageout daemon.  It scans the
 *	virtual address space, shoving out pages until the free page
 *	count has been raised by PAGEOUT_QUANTUM pages.  It then goes back
 *	to sleep.
 *
 *	This code runs in it's own thread.
 */
extern unsigned long MinFreePages;
#define PAGEOUT_QUANTUM	MinFreePages

 void
PageOutDaemon()
{
	unsigned long pmap;
	vm_offset_t	startva;
	register vm_offset_t	va;
	register vm_offset_t	pa;
	vm_offset_t	va_lvl2_align;
	register pt_entry_t	*ptep;
	register unsigned long needed;
	vm_movepage_t		pageout;
	kern_return_t r;
	extern struct map *coremap;
	
	pmap = current_pmap();
	va_lvl2_align = (vm_offset_t)i860_ptob(NPTES);
	
	va = VM_MIN_ADDRESS;
	
	while ( 1 )
	{
#if DEBUG
	printf("PageOutDaemon: sleeping\n");
#endif
	    flush_inv();
	    Wakeup( coremap );
	    Sleep( &coremap->m_name, PZERO );
	    needed = FreePageCnt + PAGEOUT_QUANTUM;
	    startva = va;
#if DEBUG
	printf("PageOutDaemon: awake, need %D, start va 0x%X\n",needed,va);
#endif
	    while ( FreePageCnt < needed )
	    {
		va += i860_PGBYTES;
		if ( va == startva )	/* Lapped system! */
			break;
	    	if ( va >= VM_MAX_ADDRESS )
			va = VM_MIN_ADDRESS;
		if ( (ptep = pmap_pte(pmap, va)) == 0)
		{
		    /*
		     *  Can only get here if the level 2 page
		     *  is missing.  Bump va up to the next
		     *  possible level 2 page block.
		     *  va_lvl2_align is the amount of virtual
		     *  space mapped by a level 2 vm page.
		     */
		    va &= ~(va_lvl2_align - 1);
		    va += va_lvl2_align;
		    va -= i860_PGBYTES;
		    continue;
		}
		/* If a page is wired or not valid, leave it alone. */
		if ( ptep->wired || ptep->valid == 0 )
			continue;

		/* If the physical memory isn't DRAM, we aren't interested. */
		pa = i860_ptob(ptep->pfn);
		if (pa < START_MANAGED_MEM || pa >= END_MANAGED_MEM)
			continue;

		/* Referenced bit set? Just clear bit and move on. */
		if ( ptep->ref )
		{
			ptep->ref = 0;
			continue;
		}
		/* We have found a DRAM page not recently referenced. */
#if DEBUG
	printf("PageOutDaemon: release va 0x%X\n",va);
#endif
		if ( ptep->modify )
		{
		    /* Set it up to be flushed to the backing store. */
		    flush();		/* Bring memory image up to date. */
		    pageout.vaddr = va;
		    pageout.paddr = pa;
		    pageout.size = i860_PGBYTES;
		    /*
		     * Set state to wired and invalid,
		     * marking page 'in transit'.
		     */
		    ptep->wired = 1;
		    ptep->valid = 0;
#if DEBUG
	printf("PageOutDaemon: flush dirty va 0x%X (pa 0x%X)\n",va, pa);
#endif
		    r = ND_Kern_page_out(ServicePort, ReplyPort, &pageout, 1);
		    if ( r != KERN_SUCCESS ) {
			    printf( "ND_Kern_page_out returns %D\n", r );
			    panic( "PageOutDaemon" );
		    }
		    ptep->modify = 0;	/* Page cleaned */
		    ptep->backing = 1;	/* Page in from backing store*/
		    ptep->wired = 0; 	/* Unlock page */	
		}
		else
		    ptep->valid = 0;
		ptep->pfn = 0;
		ptep->ref = 0;
		invalidate_TLB();
		pa = phystokv(pa);
		kmem_free( kernel_map, i860_PGBYTES, pa );
		Wakeup( va );	/* In case vm_fault() is waiting */
	    }
	}
}



/*
 * A debugging tool: Spill the contents of the page directory and PTEs
 */
pmap_dump()
{
	pmap_dump_range( (vm_offset_t)i860_PGBYTES, (vm_offset_t)0 );
}
pmap_dump_range(vm_offset_t start, vm_offset_t end)
{
	register vm_offset_t	va, va_lvl2_align;
	register pt_entry_t	*ptep;
	unsigned long	pmap;
	unsigned int i;

	pmap = current_pmap();
	for (va = start; va != 0 && va != trunc_page(end); va += i860_PGBYTES) {
		if ((ptep = pmap_pte(pmap, va)) == 0) {
			/*
			 *  Can only get here if the level 2 page
			 *  is missing.  Bump va up to the next
			 *  possible level 2 page block.
			 *  va_lvl2_align is the amount of virtual
			 *  space mapped by a level 2 vm page.
			 */
			va_lvl2_align = (vm_offset_t)i860_ptob(NPTES);
			va &= ~(va_lvl2_align - 1);
			va += va_lvl2_align;
			va -= i860_PGBYTES;
			continue;
		}
		if ( *((int *)ptep) == 0 )
			continue;
		printf( "0x%X: 0x%X ", va, *((int *)ptep) );
		if ( ptep->wired )
			printf( "wired " );
		if ( ptep->allocated )
			printf( "alloc " );
		if ( ! ptep->backing )
		{
		    if ( ! ptep->wired )
			printf( "zero " );
		}
		else
			printf( "back " );
		if ( ptep->modify )
			printf( "D " );
		if ( ptep->ref )
			printf( "A " );
		if ( ptep->cache )
			printf( "CD " );
		if ( ptep->writethru )
			printf( "WT " );
		i = ptep->prot;
		if ( i & 2 )
			printf( "U " );
		if ( i & 1 );
			printf( "W " );
		if ( ptep->valid )
			printf( "P ");
		printf( "\n" );
	}
	return;
}