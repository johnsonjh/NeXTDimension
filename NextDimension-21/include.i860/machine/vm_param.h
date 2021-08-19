/*
 ****************************************************************
 * Mach Operating System
 * Copyright (c) 1987 Carnegie-Mellon University
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 ****************************************************************
 */

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

#define trunc_page(x)		i860_trunc_page(x)
#define round_page(x)		i860_round_page(x)

#define	VM_MIN_ADDRESS		((vm_offset_t) 0)
#define VM_MIN_TEXT_ADDRESS	((vm_offset_t) 0x1000)
#define VM_MAX_ADDRESS		((vm_offset_t) 0xFF000000)
#define VM_MIN_KERNEL_ADDRESS	((vm_offset_t) 0xFF800000)
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
 *	Convert page table entry to kernel virtual address
 */
#define ptetokv(a)	(i860_ptob(((a)^1).pfn))
#define phystokv(a)	(a)		/* Mapped 1-1 in kernel virt addr space */

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
#define PTE_PFN		0xFFFFF000

#define VLO	(PTE_DIRTY + PTE_ACCESSED + PTE_PRESENT)
#define VLU	(PTE_DIRTY + PTE_ACCESSED + PTE_NOCACHE + PTE_PRESENT)
#define V_D	(PTE_DIRTY + PTE_ACCESSED + PTE_WRITABLE + PTE_PRESENT)
#define V_UD	(PTE_DIRTY + PTE_ACCESSED + PTE_NOCACHE + PTE_WRITABLE + PTE_PRESENT)


