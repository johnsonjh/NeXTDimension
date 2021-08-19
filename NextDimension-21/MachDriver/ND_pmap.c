#import <sys/types.h>
#import <kern/thread.h>
#import <kern/zalloc.h>
#import <kern/lock.h>
#import <sys/systm.h>
#import <sys/param.h>
#import <kern/kalloc.h>
#import <vm/pmap.h>
#import <vm/vm_map.h>
#import <vm/vm_kern.h>
#import <vm/vm_param.h>
#import <vm/vm_prot.h>
#import <vm/vm_page.h>
#import <next/vm_param.h>
#import <next/pcb.h>
#import <next/mmu.h>
#import <next/cpu.h>
#import <next/scr.h>	/* Machine type defs */
#import <next/vmparam.h>
#import <next/event_meter.h>
#import <nextdev/video.h>
#import <machine/spl.h>

/*
 *	Locking Protocols:
 *
 *	There are two structures in the pmap module that need locking:
 *	the pmap's themselves, and the per-page pv_lists (which are locked
 *	by locking the pv_lock_table entry that corresponds the to pv_head
 *	for the list in question.)  Most routines want to lock a pmap and
 *	then do operations in it that require pv_list locking -- however
 *	pmap_remove_all and pmap_copy_on_write operate on a physical page
 *	basis and want to do the locking in the reverse order, i.e. lock
 *	a pv_list and then go through all the pmaps referenced by that list.
 *	To protect against deadlock between these two cases, the pmap_lock
 *	is used.  There are three different locking protocols as a result:
 *
 *  1.  pmap operations only (pmap_extract, pmap_access, ...)  Lock only
 *		the pmap.
 *
 *  2.  pmap-based operations (pmap_enter, pmap_remove, ...)  Get a read
 *		lock on the pmap_lock (shared read), then lock the pmap
 *		and finally the pv_lists as needed [i.e. pmap lock before
 *		pv_list lock.]
 *
 *  3.  pv_list-based operations (pmap_remove_all, pmap_copy_on_write, ...)
 *		Get a write lock on the pmap_lock (exclusive write); this
 *		also guaranteees exclusive access to the pv_lists.  Lock the
 *		pmaps as needed.
 *
 *	At no time may any routine hold more than one pmap lock or more than
 *	one pv_list lock.  Because interrupt level routines can allocate
 *	mbufs and cause pmap_enter's, the pmap_lock and the lock on the
 *	kernel_pmap can only be held at splvm.
 */

/*
 *	Lock and unlock macros for the pmap system.  Interrupts must be locked
 *	out because mbufs can be allocated at interrupt level and cause
 *	a pmap_enter.
 */
#if	NCPUS > 1
#define READ_LOCK(s)		s = splvm(); lock_read(&pmap_lock);
#define READ_UNLOCK(s)		lock_read_done(&pmap_lock); splx(s);
#define WRITE_LOCK(s)		s = splvm(); lock_write(&pmap_lock);
#define WRITE_UNLOCK(s)		lock_write_done(&pmap_lock); splx(s);
#else	NCPUS > 1
#define READ_LOCK(s)		s = splvm();
#define READ_UNLOCK(s)		splx(s);
#define WRITE_LOCK(s)		s = splvm();
#define WRITE_UNLOCK(s)		splx(s);
#endif	NCPUS > 1

/*
 *	Lock and unlock macros for pmaps.  Interrupts must be locked out for
 *	the kernel pmap because device must read (and therefore lock) it.
 */
#define	LOCK_PMAP_U(map) \
	simple_lock(&(map)->lock);
#define	LOCK_PMAP(map,s) \
	if (map == kernel_pmap) \
		s = splvm(); \
	LOCK_PMAP_U(map);
#define	UNLOCK_PMAP_U(map) \
	simple_unlock(&(map)->lock);
#define UNLOCK_PMAP(map,s) \
	UNLOCK_PMAP_U(map); \
	if (map == kernel_pmap) \
		splx(s);

/*
 * Simulate a write reference to a page.
 */
ND_pmap_set_modify( pmap, va )
{
	extern pte_t *pmap_pte();
	pte_t	*pte;
	unsigned s;

	WRITE_LOCK(s);
	LOCK_PMAP_U(pmap);

	pte = pmap_pte_valid(pmap, va);
	if ( pte != (pte_t *)PTE_ENTRY_NULL )
	{
		pte->pte_modify = 1;
		pte->pte_refer = 1;
	}

	UNLOCK_PMAP_U(pmap);
	WRITE_UNLOCK(s);
	pmap_update();	/* Ensure that modify bit is correctly reflected. */
}
