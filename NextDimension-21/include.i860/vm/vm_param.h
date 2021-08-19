/*
 ****************************************************************
 * Mach Operating System
 * Copyright (c) 1987 Carnegie-Mellon University
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 ****************************************************************
 */
#if ! defined(__VM_VM_PARAM_H__)
#define __VM_VM_PARAM_H__

#include <i860/vm_types.h>

#define BYTE_SIZE	8	/* byte size in bits */

#define i860_PGBYTES	4096	/* bytes per i860 page */
#define i860_PGSHIFT	12	/* number of bits to shift for pages */

/*
 *	Convert bytes to pages and convert pages to bytes.
 *	No rounding is used.
 */

#define	i860_btop(x)		(((unsigned)(x)) >> i860_PGSHIFT)
#define	i860_ptob(x)		(((unsigned)(x)) << i860_PGSHIFT)

/*
 *	Round off or truncate to the nearest page.  These will work
 *	for either addresses or counts.  (i.e. 1 byte rounds to 1 page
 *	bytes.
 */

#define i860_round_page(x)	((((unsigned)(x)) + i860_PGBYTES - 1) & \
					~(i860_PGBYTES-1))
#define i860_trunc_page(x)	(((unsigned)(x)) & ~(i860_PGBYTES-1))

/*
 * Undefine things that might have been defined due to bozoisms.
 */
#if defined(round_page)
#undef round_page
#undef trunc_page
#endif
#if defined(PAGE_SIZE)
#undef PAGE_SIZE
#undef PAGE_SHIFT
#undef PAGE_MASK
#endif
#if defined(VM_MIN_TEXT_ADDRESS)
#undef VM_MIN_ADDRESS
#undef VM_MIN_TEXT_ADDRESS
#undef VM_MAX_ADDRESS
#undef VM_MIN_KERNEL_ADDRESS
#undef VM_MAX_KERNEL_ADDRESS
#endif
/* We need to decouple logical page sizes from the i860 page sizes */
extern vm_size_t	page_size;
extern vm_size_t	page_mask;
extern int		page_shift;
#define PAGE_SIZE		page_size		/* Yup, it's a variable! */
#define PAGE_SHIFT		page_shift
#define PAGE_MASK		page_mask

#define round_page(x)	((((unsigned)(x)) + PAGE_MASK) & \
					~(PAGE_MASK))
#define trunc_page(x)	(((unsigned)(x)) & ~(PAGE_MASK))
#define	btop(x)		(((unsigned)(x)) >> PAGE_SHIFT)
#define	ptob(x)		(((unsigned)(x)) << PAGE_SHIFT)


#define	VM_MIN_ADDRESS		((vm_offset_t) 0)
#define VM_MIN_TEXT_ADDRESS	((vm_offset_t) 0x2000)
#define VM_MAX_ADDRESS		((vm_offset_t) 0xF0000000)
#define VM_MIN_KERNEL_ADDRESS	((vm_offset_t) 0xF8000000)
#define VM_MAX_KERNEL_ADDRESS	((vm_offset_t) 0xffffffff)

#define	KERNEL_STACK_SIZE	(1*i860_PGBYTES)
#define INTSTACK_SIZE		(1*i860_PGBYTES)	/* interrupt stack size */

#define i860_OFFMASK	0xfff	/* offset within page */
#define PDESHIFT	22	/* page descriptor shift */
#define PDEMASK		0x3ff	/* mask for page descriptor index */
#define PTESHIFT	12	/* page table shift */
#define PTEMASK		0x3ff	/* mask for page table index */

/*
 *	Convert address offset to page descriptor index
 *
 *	Note that since the PDE is little-endian and our data is big-endian,
 * 	a small amount of magic is needed.  The XOR covers the conversion.
 */
#define pdenum(a)	((((unsigned)(a) >> PDESHIFT) & PDEMASK)^1)

/*
 *	Convert page descriptor index to user virtual address
 */
#define pdetova(a)	((vm_offset_t)((a)^1) << PDESHIFT)

/*
 *	Convert address offset to page table index
 *
 *	Note that since the PTE is little-endian and our data is big-endian,
 * 	a small amount of magic is needed.  The XOR covers the conversion.
 */
#define ptenum(a)	((((unsigned)(a) >> PTESHIFT) & PTEMASK)^1)
/*
 * Really ugly macro to let us increment sequentially through a block
 * of PTEs.  Once again, little-endian vs big_endian rears it's ugly head.
 */
#define INCR_PT_ENTRY(a)	if(1){	\
				register unsigned off = (unsigned)(a);	\
				off ^= 4;	\
				off += 4;	\
				off ^= 4;	\
				(a) = (pt_entry_t *)off; \
			} else
				
/*
 *	Convert page table entry to kernel virtual address.
 *	All DRAM is mapped into 0xF0000000-0xFFFFFFFF.  We just 
 *	OR 0xF over the nibble holding the slot for physical addresses.
 */
#define ptetokv(a)	((i860_ptob(((a)).pfn)) | 0xF0000000)
#define phystokv(a)	(((unsigned)(a)) | 0xF0000000)

/*
 *	Conversion between i860 pages and VM pages
 */

#define trunc_i860_to_vm(p)	(atop(trunc_page(i860_ptob(p))))
#define round_i860_to_vm(p)	(atop(round_page(i860_ptob(p))))
#define vm_to_i860(p)		(i860_btop(ptoa(p)))
/*
 * Macros to convert pointers between little-endian PTE/PDE space and big-endian
 * data space.
 *
 * Usage:	ptr = l_to_b(ptr, int *);
 */
#define l_to_b(addr, type)	((type)(((unsigned)(addr))^4))
#define b_to_l(addr, type)	((type)(((unsigned)(addr))^4))

/*
 * Bits to be set within the PDE/PTE
 */
#define PTE_PRESENT	0x001
#define PTE_WRITABLE	0x002
#define PTE_USER	0x004
#define PTE_WRITETHRU	0x008	/* On N10, cache inhibit in 2nd level PTE ONLY. */
#define PTE_NOCACHE	0x010	/* Cache Disable for 2nd level PTE ONLY. */
#define PTE_ACCESSED	0x020	/* Touched on page access */
#define PTE_DIRTY	0x040	/* Set by trap handler on write to page */
#define PTE_RESERVED1	0x080
#define PTE_RESERVED2	0x100
#define PTE_USER1	0x200
#define PTE_USER2	0x400
#define PTE_USER3	0x800
#define PTE_BACKING	PTE_USER1
#define PTE_ALLOCATED	PTE_USER2
#define PTE_WIRED	PTE_USER3
#define PTE_PFN		0xFFFFF000

#define VLO	(PTE_DIRTY + PTE_ACCESSED + PTE_PRESENT)
#define VLU	(PTE_DIRTY + PTE_ACCESSED + PTE_NOCACHE + PTE_PRESENT)
#define V_D	(PTE_DIRTY + PTE_ACCESSED + PTE_WRITABLE + PTE_PRESENT)
#define V_UD	(PTE_DIRTY + PTE_ACCESSED + PTE_NOCACHE + PTE_WRITABLE + PTE_PRESENT)

/*
 *	i860 Page Table Entry
 *
 *	This layout is correct for GCC 1.36, running BIG_ENDIAN.
 */

struct pt_entry {
	unsigned int
			pfn:20,		/* page frame number */
			wired:1,	/* user bits: page is busy, locked down. */
			allocated:1,	/* user bits: page is allocated */
			backing:1,	/* user bit: use backing store or zero fill */
			:2,		/* hardware reserved; */
			modify:1,	/* hardware modify bit */
			ref:1,		/* hardware page referenced */
			cache:1,	/* hardware cache disable */
			writethru:1,	/* N11: write thru cache policy */
			prot:2,		/* protection */
			valid:1;	/* valid: the page is in core. */
};

typedef struct pt_entry	pt_entry_t;
#define	PT_ENTRY_NULL	((pt_entry_t *) 0)


#define NPTES	(i860_ptob(1)/sizeof(pt_entry_t))
#define NPDES	(i860_ptob(1)/sizeof(pt_entry_t))

/*
 *	i860 PTE protection codes.  (To be used in conjunction with the
 *	structure above).
 */

#define i860_NO_ACCESS	0		/* no access */
#define i860_UR		2		/* user readable */
#define i860_UW		3		/* user writeable */
#define i860_KR		0		/* kernel readable */
#define i860_KRW	1		/* kernel readable, writeable */

/*
 *	i860 pte bit definitions (to be used directly on the ptes
 *	without using the bit fields).
 */

#define i860_PTE_VALID	0x00000001
#define i860_PTE_UR	0x00000004
#define i860_PTE_UW	0x00000006
#define i860_PTE_KR	0x00000000
#define i860_PTE_KRW	0x00000002
#define i860_PTE_USER	0x00000004
#define i860_PTE_WRITE	0x00000002
#define i860_PTE_PERM	0x00000006
#define i860_PTE_REF	0x00000020
#define i860_PTE_MOD	0x00000040
#define i860_PTE_WIRED	0x00000200
#define	i860_PTE_PFN	0xfffff000

#endif /*__VM_VM_PARAM_H__*/

