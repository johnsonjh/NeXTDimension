
#include "mach_load.h"

#include "sys/types.h"
#include "sys/map.h"		/* For map decls */
#include "machine/param.h"	/* For coremap size */
#include "machine/scb.h"
#include "machine/cpu.h"
#include "machine/pcb.h"
#include "ND/NDreg.h"
#include "ND/NDmsg.h"

#include "vm/vm_param.h"
#include "vm/vm_prot.h"

#include "i860/vm_types.h"
#include "kern/mach_types.h"

#define UNCACHED_START	ND_START_UNCACHEABLE_DRAM
#define UNCACHED_END	ND_END_UNCACHEABLE_DRAM


#define DEBUG	0


vm_offset_t		loadpt;

static vm_offset_t	_kernel_end_;

int		cpu;

vm_size_t	page_size;
vm_size_t	page_mask;
int		page_shift;

extern struct map *coremap;		/* declared in param.c */
extern unsigned long FreePageCnt;
/*
 *	Structure to keep track of RAM banks.
 */
struct DRAM_Bank
{
	vm_address_t addr;
	vm_size_t size;
};

#define N_DRAM_BANKS	4	/* Four 16 Mbyte banks of DRAM */
static struct DRAM_Bank DRAM_Banks[N_DRAM_BANKS];

void vm_set_page_size();
static vm_offset_t config_memory(vm_offset_t,struct DRAM_Bank *,vm_offset_t *);
static void size_memory(struct DRAM_Bank *);

/*
 * is_empty()
 *
 *	Called with the base address of a bank of memory.
 *	returns non-zero if no memory is present in that bank.
 *	Returns zero if memory is present.
 */
 static int
is_empty( vm_address_t addr )
{
	int save1, save2;
	int empty = 0;

	save1 = *((volatile int *)addr);
	save2 = *((volatile int *)(addr + 4));
	
	*((volatile int *)addr) = 0x55555555;
	*((volatile int *)(addr + 4)) = 0xaaaaaaaa;

	if ( *((volatile int *)addr) != 0x55555555 )
		++empty;

	if ( *((volatile int *)(addr + 4)) != 0xaaaaaaaa )
		++empty;

	*((volatile int *)addr) = save1;
	*((volatile int *)(addr + 4)) = save2;
	return empty;
}

/*
 * is_4Mbit()
 *
 *	Called with the base address of a bank of memory.
 *	returns non-zero if bank uses 4 Mbit parts (16 Mbyte bank).
 *	Returns zero if bank uses 1 Mbit parts (4 Mbyte bank).
 */
is_4Mbit( vm_address_t addr )
{
	register int i;
	int savedata[16];
	register int is4Mbit = 0;

	/* Save contents of RAM */
	for ( i = 0; i < 16; ++i )
		savedata[i] = *((volatile int *)(addr + (i * 0x100000)));

	/* Mark each Mbyte uniquely, from the top down. */
	for ( i = 15; i >= 0; --i )
		*((volatile int *)(addr + (i * 0x100000))) = i;

	/* Read from the bottom up.  Stop at the 1st mismatch */
	for ( i = 0; i < 16; ++i )
	{
		if ( *((volatile int *)(addr + (i * 0x100000))) != i )
			break;
	}
	if ( i == 16 )	/* No mismatches. Assume 4 Mbit parts. */
		is4Mbit = 1;

	/* Restore contents of RAM */
	for ( i = 0; i < 16; ++i )
		*((volatile int *)(addr + (i * 0x100000))) = savedata[i];
	
	return is4Mbit;
}

/*
 * Set DRAM size to 4 Mbits, so we can see all of memory.
 * Probe memory and determine the amount of memory present in each bank.
 * Fill in the DRAM_Banks table (carefully!  VM isn't running yet).
 */
 static void 
size_memory( struct DRAM_Bank * dbp )
{
	vm_address_t ramaddr;
	vm_offset_t off;
	int savedata;
	int i;
	
	*ADDR_ND_DRAM_SIZE = ND_DRAM_SIZE_4_MBIT;

	ramaddr = PADDR_DRAM;	/* Get absolute physical address of DRAM */
	off = 0;
	/* Test each bank for the presence and size of it's memory. */
	for ( i = 0; i < N_DRAM_BANKS; ++i )
	{
		dbp[i].addr = ramaddr + off;
		if ( is_empty( ramaddr + off ) )
			dbp[i].size = 0;
		else
		{
			if ( is_4Mbit( ramaddr + off ) )
				dbp[i].size = 0x01000000;	// 16 Mbytes
			else
				dbp[i].size = 0x00400000;	// 4 Mbytes
		}
		off += 0x01000000;	// Add 16 Mbytes to get to next bank
	}
}

/*
 * Dawn of time initialization operations.
 *
 *	Size memory.
 *	Note the current end of the kernel.
 *	Grow the kernel to accomodate page tables mapping the kernel.
 *	Return the physical address of the kernel page directory.
 */
early_start()
{
	struct DRAM_Bank *dbp;
	register vm_offset_t *vp;
	vm_offset_t directory;
	register vm_offset_t kern_end;
	extern char _end;
	register int i;

	/* Convert the virtual address to a valid physical addr... */
	dbp = (struct DRAM_Bank *) abs_addr( &DRAM_Banks[0] );
	
	size_memory( dbp );
#if DEBUG
	for ( i = 0; i < N_DRAM_BANKS; ++i )
	{
		puts( "DRAM Bank " );
		putx( i );
		puts( " Addr " );
		putx( dbp[i].addr );
		puts( " Size " );
		putx( dbp[i].size );
		cnputc( '\n' );
	}
#endif
	/* Get physical address of end of kernel, on a page boundry. */
	kern_end = (vm_offset_t) i860_round_page( abs_addr( &_end ) );
#if DEBUG
	puts( "initial kernel end " );
	putx( kern_end );
	cnputc( '\n' );
#endif
	kern_end = config_memory( kern_end, dbp, &directory );
#if DEBUG
	puts( "final kernel end " );
	putx( kern_end );
	cnputc( '\n' );
#endif
	vp = (vm_offset_t *) abs_addr( &_kernel_end_ );
	*vp = kern_end;
	return directory;	// Code in _startup uses this to load dirbase
}

i860_start()
{
	int i;
	vm_size_t freecore;
	vm_size_t corepages;
	int coremap_size;
	vm_size_t need;
	extern char start, vstart;

#if DEBUG
	printf( "i860_start\n" );
#endif
	/* Initialize misc parameters... */
	vm_set_page_size();
	cpu = CPU_NextDimension;

	/*
	 * Prepare the coremap for use.  Estimate the map size needed,
	 * and steal sufficient pages off the end of the kernel.
	 */
	freecore = 0;
	for ( i = 0; i < N_DRAM_BANKS; ++i )
		freecore += DRAM_Banks[i].size;
	/* Subtract wired kernel size from corepages */
	freecore -= phystokv(_kernel_end_) - ((vm_offset_t)&vstart);
	freecore -= ND_END_UNCACHEABLE_DRAM - ND_START_UNCACHEABLE_DRAM;
	/* Convert to a page count */
	corepages = btop(freecore);
	/* Under worst case conditions, we need corepages/2 entries in the coremap. */
	coremap_size = corepages >> 1;			/* Divide by 2 */
	/* Steal memory for the coremap off the end of the kernel */
	need = round_page(coremap_size * sizeof (struct mapent));
	coremap = (struct map *) phystokv(_kernel_end_);
	_kernel_end_ += need;
	freecore -= need;
	coremap_size = need / sizeof (struct mapent);	/* Let it use all of page. */
#if DEBUG
	printf( "coremap uses %D bytes starting at 0x%X\n", need, coremap );
#endif
	
	/* Initialize the core map and kernel memory allocator */
	rminit( coremap, coremap_size, (char *)"coremap" );
	kmem_init();
	
	/*
	 * Free all unused DRAM into the core map.
	 */
	/* Free the space between _end of the kernel and start of the msg buffers. */
	rmfree( coremap,
		btop(ND_START_UNCACHEABLE_DRAM - phystokv(_kernel_end_)),
		btop(phystokv(_kernel_end_)) );
	/* Free the rest of Bank 0 */
	rmfree( coremap,
		btop(DRAM_Banks[0].size -
			(ND_END_UNCACHEABLE_DRAM - (vm_size_t)ADDR_DRAM)),
		btop(ND_END_UNCACHEABLE_DRAM) );
	/* Free remaining banks... */
	for ( i = 1; i < N_DRAM_BANKS; ++i )
	{
	    if ( DRAM_Banks[i].size != 0 )
	    {
		rmfree(coremap,
			btop(DRAM_Banks[i].size),
			btop(phystokv(DRAM_Banks[i].addr)));
	    }
	}
	/*
	 * Arm interrupts in memory controller. Clear any pending ones.
	 * The initialization routines that follow set conditions up that
	 * let the host know it's OK to interrupt the i860.  The i860 won't
	 * process these interrupts yet, though.
	 *
	 * We do this by writing the interrupt enable bits in MC CSR 0.
	 * Don't switch on the video I/O interrupt enable.  That is done by the 
	 * live video control package.
	 */
	*ADDR_ND_CSR = NDCSR_INT860ENABLE | NDCSR_BE_IEN | NDCSR_VBL_IEN;
	
	pmap_init();		/* Configure the page table stuff */
	InitPorts();		/* Configure the port interface. */
	InitMessages();		/* Configure the message system host interface */
	InitCEQ();
	/*
	 * Free the 1:1 mapped boot code block.  This is the stuff from 
	 * symbol 'start' to symbol 'vstart' in locore.s
	 */
	pmap_remove(current_pmap(), kvtophys(&start), kvtophys(&vstart) );

	if ( KernelDebugEnabled() )
	{
	    printf("NextDimension Graphics Accelerator Kernel: free memory: %D bytes\n",
	    	ptob(FreePageCnt) );
	}

	if ( KernelDebugEnabled() )
	    kdb(0, 0, 0);

	cache_on();
#if DEBUG
	printf( "Cache on...\n" );
#endif
	spl0();	/* Permit hardware interrupts. */
}

/*
 * Set the logical page size.  This should be the lowest common whole product of
 * the host and i860 page sizes.
 *
 * Currently, the i860 uses a 4 Kbyte page and the host uses an 8 Kbyte page.
 */
 void
vm_set_page_size()
{
	int i;
	page_size = ND_PAGE_SIZE;	/* Tunable parameter. */
	page_mask = page_size - 1;
	
	page_shift = 0;
	for( i = page_size; (i & 1) == 0; i >>= 1 )
		++page_shift;
}

/* PTE mode bits for assorted areas of memory. */
#define PTE_CSR		V_UD		/* Hardware CSR page */
#define PTE_VRAM	V_D		/* The frame buffer */
#define PTE_DATA	V_D		/* Mode for misc processor data */
#define PTE_UNCACHED_DATA	V_UD	/* Assorted data which MUST not be cached */
#define PTE_TEXT	V_D		/* Mode for executable code */
#define PTE_PTES	V_UD		/* Mode for pages of PTEs */
#define PTE_KPDE	V_D		/* Mode for page directory entries. */

/*
 * Map all available DRAM into the kernel address space, using a linear
 * physical to virtual mapping.  The kernel needs access to all pages
 * of core so that they can be filled in correctly later as pages are
 * needed to back VM.
 *
 * This function is called before VM is active, so we can't just use the
 * usual pmap interface.
 */
 static vm_offset_t
config_memory(vm_offset_t start_free, struct DRAM_Bank *dbp, vm_offset_t *pde)
{
	vm_offset_t off, voff;
	unsigned page;
	unsigned long *kpde;
	unsigned long *kpte;
	int i;
	extern char stext, _etext, bad, start, vstart;
	
	/* Allocate kernel page directory. */
	page = start_free;
	start_free += i860_PGBYTES;
#if DEBUG
	puts("PDE ");putx(page);cnputc('\n');
#endif
	*pde = (vm_offset_t) page;
	kpde = (unsigned long *) page;
	bzero( (char *) kpde, i860_PGBYTES );

	/* Setup I/O pages.  These are known to take 1 PDE */
	page = start_free;
	start_free += i860_PGBYTES;
#if DEBUG
	puts("MC CSR PTE ");putx(page);cnputc('\n');
#endif
	kpte = (unsigned long *)page;
	bzero( (char *) kpte, i860_PGBYTES );
	/* Cache Disable bit is undefined in 1st level PDE */
	kpde[pdenum(ND_CSR_VIRT_REGS)] = ((long)kpte) | PTE_KPDE;
	for ( off = 0; off < PADDR_MC_SIZE; off += i860_PGBYTES )
	{
	    kpte[ptenum(ND_CSR_VIRT_REGS + off)] =
		    i860_trunc_page(ND_CSR_REGS + off) | PTE_CSR;
	}
	

	/* Fill in 2nd level PTEs for DRAM. */
	for ( i = 0; i < N_DRAM_BANKS; ++i )
	{
	    for (off = dbp[i].addr; off < dbp[i].addr+dbp[i].size; off += i860_PGBYTES)
	    {
	    	voff = phystokv(off);
		if ((kpte = (unsigned long *)kpde[pdenum(voff)]) == (unsigned long *)0)
		{
		    page = start_free;
		    start_free += i860_PGBYTES;
#if DEBUG
		    puts("DRAM PTE ");putx(page);cnputc('\n');
#endif
		    kpte = (unsigned long *)page;
		    bzero( (char *) kpte, i860_PGBYTES );
		    kpde[pdenum(voff)] = ((unsigned long)kpte) | PTE_KPDE; 
		}
		kpte = (unsigned long *)i860_trunc_page(kpte);
		kpte[ptenum(voff)] = i860_trunc_page(off) | PTE_DATA;
	    }
	}
	/*
	 * Special case:  The first pages of memory, from 'start' to 'vstart', 
	 * are to be mapped 1:1.  These pages are only active when we are
	 * booting the system software (NOW!)
	 */
	for ( off = abs_addr(&start); off < abs_addr(&vstart); off += i860_PGBYTES )
	{
	    if ((kpte = (unsigned long *)kpde[pdenum(off)]) == (unsigned long *)0)
	    {
		page = start_free;
		start_free += i860_PGBYTES;
#if DEBUG
		puts("1:1 DRAM PTE ");putx(page);cnputc('\n');
#endif
		kpte = (unsigned long *)page;
		bzero( (char *) kpte, i860_PGBYTES );
		kpde[pdenum(off)] = ((unsigned long)kpte) | PTE_KPDE; 
	    }
	    kpte = (unsigned long *)i860_trunc_page(kpte);
	    kpte[ptenum(off)] = i860_trunc_page(off) | PTE_UNCACHED_DATA;
	}
	/*
	 * Special case: The last page in the virtual address space needs to have
	 * RAM placed behind it.  This page holds the trap handler vector.
	 *
	 * This page is marked as uncacheable.  We use it as a target for a dummy store,
	 * 'st.l r0,-4(r0)',  to implement workaround #3, for i860 CHIP BUG #46.
	 */
	off = start_free;
	voff = i860_trunc_page(ADDR_TRAP_VECTOR);
	start_free += i860_PGBYTES;
#if DEBUG
	puts("TRAP PAGE ");putx(off);cnputc('\n');
#endif
	if ((kpte = (unsigned long *)kpde[pdenum(voff)]) == (unsigned long *)0)
	{
	    page = start_free;
	    start_free += i860_PGBYTES;
#if DEBUG
	    puts("TRAP PTE ");putx(page);cnputc('\n');
#endif
	    kpte = (unsigned long *)page;
	    bzero( (char *) kpte, i860_PGBYTES );
	    kpde[pdenum(voff)] = ((unsigned long)kpte) | PTE_KPDE; 
	}
	kpte = (unsigned long *)i860_trunc_page(kpte);
	kpte[ptenum(voff)] = i860_trunc_page(off) | PTE_UNCACHED_DATA;
	
	/*
	 * non-cacheable I/O pages:
	 *	these are already allocated, as part of DRAM.  Modify the
	 *	state of each entry to reflect it's being non-cacheable.
	 */
	for ( off = UNCACHED_START; off < UNCACHED_END; off += i860_PGBYTES )
	{
	    kpte = (unsigned long *)i860_trunc_page( kpde[pdenum(off)] );
	    kpte[ptenum(off)] = i860_trunc_page(kpte[ptenum(off)]) | PTE_UNCACHED_DATA;
	}
	
	/*
	 * Pages which hold page table entries must also be uncacheable.
	 * Look through the page directory for valid entries.  Modify the 
	 * PTE pointing to that PDE to keep it from being cached.
	 *
	 * We have to convert the physical address in the directory to a virtual addr.
	 */
	for ( i = 0; i < i860_PGBYTES/sizeof(unsigned long); ++i )
	{
	    if ( (off = (vm_offset_t)i860_trunc_page( kpde[i] )) != 0 )
	    {
		voff = phystokv(off);
		kpte[ptenum(voff)] = i860_trunc_page(kpte[ptenum(voff)]) | PTE_PTES;
	    }
	}
	/* Make the Page Directory page uncacheable. */
	voff = phystokv( kpde );
	kpte[ptenum(voff)] = i860_trunc_page(kpte[ptenum(voff)]) | PTE_PTES;

	/* Finally, set up the mode for kernel text pages. */
	for(off = trunc_page(&vstart); off < round_page(&_etext); off += i860_PGBYTES)
	    kpte[ptenum(off)] = i860_trunc_page(kpte[ptenum(off)]) | PTE_TEXT;
	
	return start_free;
}

