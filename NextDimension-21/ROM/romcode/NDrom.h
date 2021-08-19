/*
 * Main definitions file for the NextDimension boot ROM
 */

/*
 *	Structure to keep track of RAM banks.
 */
struct DRAM_Bank
{
	unsigned long addr;
	unsigned long size;
};

#define N_DRAM_BANKS	4	/* Four 16 Mbyte banks of DRAM */


/*
 * Global memory structure for the ND boot ROM.
 */
 
struct ROMGlobals
{
	struct DRAM_Bank	DRAM_Banks[N_DRAM_BANKS];
};

/*
 *	The ROMGlobals pointer, in global register r15
 */
register struct ROMGlobals * ROMGlobals asm("r15");
