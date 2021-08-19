/*
 * Copyright 1989 NeXT, Inc.
 *
 * NextDimension Device Driver
 *
 * This module contains the code used to erase and program the EEPROM
 * on the NextDimension board.
 *
 * This stuff has to be done from the device driver due to some really
 * hairy real time requirements.
 */

#include "ND_patches.h"			/* Must be FIRST! */
#include <nextdev/slot.h>
#include <next/pcb.h>
#include <machine/spl.h>
#include <sys/param.h>
#include <ND/NDmsg.h>
#include "ND_var.h"
#include <sys/mig_errors.h>

extern ND_var_t ND_var[];

/* Local functions. */
static int program_addr( volatile unsigned char *, unsigned char );


#define ND_SLOT_ADDRESS(unit)	(0xF0000000 | ((unit)<<24))
#define ND_SLOT_WIDTH		0x01000000

/*
 * The following macro converts an address from the generic form to a
 * slot-specific format.  It also includes a workaround for the first
 * version of the memory controller chip on the X4 and later boards.
 * This workaround supplies A0 and A1 addresses to the chip from address
 * lines A17 and A18.  A0 and A1 of the final address asserted to the
 * chip MUST BE 0.
 */

/*
 * Commands specific to the 28F010 EEPROM.
 */
#define CMD_READ_MEM		0x00
#define CMD_READ_ID		0x90
#define CMD_ERASE_SETUP		0x20
#define CMD_ERASE		0x20
#define CMD_ERASE_VERIFY	0xA0
#define CMD_PROG_SETUP		0x40
#define CMD_PROG		0x40
#define CMD_PROGRAM_VERIFY	0xC0
#define CMD_RESET		0xFF

/* "Generic" addresses in the ROM */
#define ADDR_ROM_BASE		0xFFFE0000
#define ADDR_MFR_ID		ADDR_ROM_BASE
#define ADDR_DEV_ID		(ADDR_ROM_BASE + 1)

#define ROM_SIZE		0x00020000
#define START_CHECKSUM_OFF	4	// 1st 4 bytes are the checksum itself

/*
 *	The FIX_ADDR macro does several things.
 *
 *	First, it masks out the slot ID portion of the supplied
 *	address and ORs in the corrrect slot.
 *	Second, it moves the low 2 bits of the byte address
 *	up to A17 and A18, for the 1st generation memory controller bug 
 *	workaround.
 *	Third, it masks out A19 of the address, forcing the true memory
 *	address to be an alias of the ROM location used by the i860 and host,
 *	and avoiding the 'hole' in the ROM address space caused by the NBIC 
 *	interrupt and interrupt mask registers.
 */
#define FIX_ADDR(a,u)	((((a) & ~0x0F0E0003)|((a)&3)<<17)|((u)<<24))

/* Timer values, in microseconds. */
#define TIME_PROGRAM	10	/* 10 uSec minimum. */
#define TIME_VERIFY	6
#define TIME_ERASE	9500	/* 9.5 mSec minimum. */

/* Retry counts for programming and erase algorithms. */
#define RETRY_PROGRAM	15
#define RETRY_ERASE	128 /* Doc says 64 in one place, 1000 in another. */

 kern_return_t
ND_ROM_Identifier(
	port_t		nd_port,
	int		unit,
	unsigned	*manufacturer,
	unsigned	*device)
{
	kern_return_t 	r;
	volatile unsigned char	*addr;
	unsigned char mfg, dev;

	/*
	 * Ensure that arg makes sense.
	 */
	if ( UNIT_INVALID( unit ) )
		return KERN_INVALID_ARGUMENT;

	/* Activate transparent translation for board slot space. */
	pmap_tt(current_thread(), 1, ND_SLOT_ADDRESS(unit), ND_SLOT_WIDTH, 0);
	
	/* Set pointer to the first address to be read. */
	addr = (volatile unsigned char *)FIX_ADDR(ADDR_MFR_ID, unit);
	
	/* Check for a ROM, out of paranoia */
	if ( probe_rb( addr ) == 0 )
	{
		pmap_tt(current_thread(), 0, 0, 0, 0);
		return KERN_INVALID_ADDRESS;
	}
	*addr = CMD_READ_ID;	/* Issue command to read ID info */
	mfg = *addr;
	addr = (volatile unsigned char *)FIX_ADDR(ADDR_DEV_ID, unit);
	dev = *addr;
	
	*addr = CMD_READ_MEM;	/* Issue command to restore normal operation */

	/* Shut down transparent translation. */
	pmap_tt(current_thread(), 0, 0, 0, 0);
	*manufacturer = mfg;
	*device = dev;
	
	return KERN_SUCCESS;
}

 kern_return_t
ND_ROM_CheckSum(
	port_t		nd_port,
	int		unit,
	unsigned	*checksum,
	unsigned	*record_sum)
{
	kern_return_t 	r;
	volatile unsigned char	*addr;
	unsigned int off, sum;

	/*
	 * Ensure that arg makes sense.
	 */
	if ( UNIT_INVALID( unit ) )
		return KERN_INVALID_ARGUMENT;

	/* Activate transparent translation for board slot space. */
	pmap_tt(current_thread(), 1, ND_SLOT_ADDRESS(unit), ND_SLOT_WIDTH, 0);
	
	/* Set pointer to the first address to be read. */
	addr = (volatile unsigned char *)FIX_ADDR(ADDR_ROM_BASE, unit);
	
	/* Check for a ROM, out of paranoia */
	if ( probe_rb( addr ) == 0 )
	{
		pmap_tt(current_thread(), 0, 0, 0, 0);
		return KERN_INVALID_ADDRESS;
	}
	*addr = CMD_READ_MEM;	/* Issue command to select normal operation */

	/* Extract the recorded checksum from the ROM */
	sum = (*addr << 24);
	addr = (volatile unsigned char *)FIX_ADDR(ADDR_ROM_BASE+1, unit);
	sum |= (*addr << 16);
	addr = (volatile unsigned char *)FIX_ADDR(ADDR_ROM_BASE+2, unit);
	sum |= (*addr << 8);
	addr = (volatile unsigned char *)FIX_ADDR(ADDR_ROM_BASE+3, unit);
	sum |= *addr;
	*record_sum = sum;
	
	/* Compute a checksum for the ROM */
	sum = 0;
	for ( off = START_CHECKSUM_OFF; off < ROM_SIZE; ++off )
	{
	    addr = (volatile unsigned char *)FIX_ADDR(ADDR_ROM_BASE+off, unit);
	    sum += *addr;
	}
	/* Shut down transparent translation. */
	pmap_tt(current_thread(), 0, 0, 0, 0);
	*checksum = sum;
	
	return KERN_SUCCESS;
}


 kern_return_t
ND_ROM_Erase(
	port_t		nd_port,
	int		unit)
{
	kern_return_t 	r;
	volatile unsigned char	*addr;
	int		i;
	unsigned	logical_addr;
	unsigned	last_verified_offset;
	unsigned	s;
	int		pulse_count;
	unsigned char	datum;
	
	/*
	 * Ensure that arg makes sense.
	 */
	if ( UNIT_INVALID( unit ) )
		return KERN_INVALID_ARGUMENT;

	/* Activate transparent translation for board slot space. */
	pmap_tt(current_thread(), 1, ND_SLOT_ADDRESS(unit), ND_SLOT_WIDTH, 0);
	
	/* Set pointer to the first address to be read. */
	addr = (volatile unsigned char *)FIX_ADDR(ADDR_ROM_BASE, unit);
	
	/* Check for a ROM, out of paranoia */
	if ( probe_rb( addr ) == 0 )
	{
	    pmap_tt(current_thread(), 0, 0, 0, 0);
	    return KERN_INVALID_ADDRESS;
	}

	/*
	 * Intel Quick-Erase Algorithm
	 * Vpph is already applied.
	 */
	
	/* Program all bytes to 0 */
	logical_addr = ADDR_ROM_BASE;
	for ( i = 0; i < ROM_SIZE; ++i )
	{
	    addr = (volatile unsigned char *)FIX_ADDR(logical_addr, unit);
	    if ( *addr != 0 )
	    {
		if ( ! program_addr( addr, 0 ) )
		    goto erase_failure;
	    }
	    ++logical_addr;
	}
	
	last_verified_offset = 0;
	pulse_count = 0;
	
	while ( pulse_count++ < RETRY_ERASE )
	{
	    addr = (volatile unsigned char *)
	    		FIX_ADDR(ADDR_ROM_BASE + last_verified_offset, unit);
	    *addr = CMD_ERASE_SETUP;
	    s = splsched();		/* Begin critical section */
	    *addr = CMD_ERASE;		/* Start erasure pulse */
	    DELAY(TIME_ERASE);
	    *addr = CMD_ERASE_VERIFY;	/* End erase pulse, start verify */
	    splx(s);			/* End critical section */
	    DELAY(TIME_VERIFY);
	    datum = *addr;		/* Read datum to verify erasure */
	    if ( datum != 0xFF )
	    	continue;
	    /* Verify additional bytes until we hit a bad one. */
	    while ( ++last_verified_offset < ROM_SIZE )
	    {
		addr = (volatile unsigned char *)
	    		FIX_ADDR(ADDR_ROM_BASE + last_verified_offset, unit);
		*addr = CMD_ERASE_VERIFY; /* start verify */
		DELAY(TIME_VERIFY);
		datum = *addr;		/* Read datum to verify erasure */
		if ( datum != 0xFF )
		    break;		/* Found a bad one, so erase again. */
	    }
	    if ( last_verified_offset == ROM_SIZE )
	    {
		/* Issue command to restore normal operation */
		*addr = CMD_READ_MEM;
		/* Shut down transparent translation. */
		pmap_tt(current_thread(), 0, 0, 0, 0);
		return KERN_SUCCESS;
	    }
	}
	
erase_failure:
	*addr = CMD_READ_MEM;	/* Issue command to restore normal operation */
	/* Shut down transparent translation. */
	pmap_tt(current_thread(), 0, 0, 0, 0);
	return KERN_FAILURE;
}

/*
 *	Program a block of data into the ROM.
 *
 *	Up to 1 Kbyte may be programmed per call.
 *	The programming data is passed in-line to
 *	avoid all of the hairballs implicit in out of line
 *	messages to a loadable device driver.
 */
 kern_return_t
ND_ROM_Program(
	port_t		nd_port,
	int		unit,
	unsigned long	start_addr,
	unsigned char	*data,
	unsigned long 	dataCnt )
{
	ND_var_t		*sp;
	kern_return_t 		r;
	volatile unsigned char	*addr;
	
	/*
	 * Test for out of range args.  This is more than a little strange
	 * due to the ROM lying at the very top of the address space, with
	 * modulo 32 wraparound and all that...
	 */
	if (start_addr < ADDR_ROM_BASE )
		return KERN_INVALID_ARGUMENT;
	if ( (start_addr+dataCnt) != 0 && (start_addr+dataCnt) < ADDR_ROM_BASE)
		return KERN_INVALID_ARGUMENT;
	if ( UNIT_INVALID( unit ) )
		return KERN_INVALID_ARGUMENT;

	/* Activate transparent translation for board slot space. */
	pmap_tt(current_thread(), 1, ND_SLOT_ADDRESS(unit), ND_SLOT_WIDTH, 0);
	
	/* Set pointer to the first address to be read. */
	addr = (volatile unsigned char *)FIX_ADDR(start_addr, unit);
	
	/* Check for a ROM, out of paranoia */
	if ( probe_rb( addr ) == 0 )
	{
	    pmap_tt(current_thread(), 0, 0, 0, 0);
	    return KERN_INVALID_ADDRESS;
	}

	while ( dataCnt-- )
	{
	    addr = (volatile unsigned char *)FIX_ADDR(start_addr, unit);
	    ++start_addr;
	    if ( ! program_addr( addr, *data++ ) )
	    {
		/* Issue command to restore normal operation */
		*addr = CMD_READ_MEM;
		/* Shut down transparent translation. */
		pmap_tt(current_thread(), 0, 0, 0, 0);
		return KERN_FAILURE;
	    }
	}

	*addr = CMD_READ_MEM;	/* Issue command to restore normal operation */
	/* Shut down transparent translation. */
	pmap_tt(current_thread(), 0, 0, 0, 0);
	return KERN_SUCCESS;
}

/*
 * Program one location. addr is the actual VM address to use.
 *
 * Refer to the Intel Quick-Pulse Programming documents for details.
 */
 static int
program_addr( volatile unsigned char *addr, unsigned char datum )
{
	unsigned s;
	int pulse_count = 0;
	unsigned char verify_datum;
	
	/* Vpph is already applied to the part. */
	while ( pulse_count++ < RETRY_PROGRAM )
	{
		s = splhigh();	/* Begin critical section. */
		/* Write Set-up Program Command */
		*addr = CMD_PROG_SETUP;
		/* Write the data to the address to be programmed. */
		*addr = datum;
		/* Wait 10 microseconds */
		DELAY(TIME_PROGRAM);
		/* Terminate program pulse, begin verify */
		*addr = CMD_PROGRAM_VERIFY;
		splx(s);	/* End critical section. */
		/* Wait 6 microseconds */
		DELAY(TIME_VERIFY);
		verify_datum = *addr;
		if ( verify_datum == datum )
			return 1;	/* Programming op succeeds */
	}
	return 0;	/* Addr failed to program after 15 tries. */
}