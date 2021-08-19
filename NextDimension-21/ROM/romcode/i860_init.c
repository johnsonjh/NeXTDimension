#include "NDrom.h"
#include "mach_load.h"

#include "sys/types.h"
#include "sys/map.h"		/* For map decls */
#include "machine/param.h"	/* For coremap size */
#include "machine/scb.h"
#include "machine/cpu.h"
#include "machine/pcb.h"
#include "ND/NDreg.h"

#include "vm/vm_param.h"
#include "vm/vm_prot.h"

#include "i860/vm_types.h"
#include "kern/mach_types.h"


#define DEBUG	0


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
 *	Find the end of the last region of memory, and make that the
 *	globals area.  Put the stack just under the globals area.
 */
 void *
early_start()
{
	int i;
	struct DRAM_Bank *dbp;
	struct DRAM_Bank DRAM_Banks[N_DRAM_BANKS];
	unsigned char *end = (unsigned char *)0;

	dbp = &DRAM_Banks[0];
	
	size_memory( dbp );
	
	for ( i = 0; i < N_DRAM_BANKS; ++i )
	{
	    if ( DRAM_Banks[i].size != 0 )
		end = (unsigned char *)(DRAM_Banks[i].addr+DRAM_Banks[i].size);
	}
	ROMGlobals = (struct ROMGlobals *) end;
	--ROMGlobals;
	//
	// The ROMGlobals area is in the highest available memory now.
	// Fill in the memory layout information, for diagnostic test use.
	//
	for ( i = 0; i < N_DRAM_BANKS; ++i )
		ROMGlobals->DRAM_Banks[i] = DRAM_Banks[i];
	//
	// Return an address to use for the top of the stack.
	// The stack grows down from this address.
	//
	return ( (void *)ROMGlobals );
}

