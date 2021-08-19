/*
 * Copyright 1989 NeXT, Inc.
 *
 * Simple loadable kernel server example.
 *
 * This module contains the code which sends a message out the reply port
 * on receipt of an interrupt from the NextDimension board.
 */
#include "ND_patches.h"			/* Must be FIRST! */
#include <nextdev/slot.h>
#include <next/pcb.h>
#include <next/mmu.h>
#include <next/scr.h>
#include "ND_var.h"

#include "ND_gamma.h"

/*
 * Put the hardware into a sane state
 * This code is called at driver init time, when we don't have any mapping set 
 * up yet.
 *
 *	1)	Stop the **%^&& blinking RAM DACs.
 *	2)	Load a reasonable CLUT.
 *	3)	Load a reasonable screen (Grey!), unblank production boards
 *	4)	Configure the message buffers.
 */
#if defined(PROTOTYPE)
void ND_init_hardware (
		ND_var_t *sp )
{
	volatile int *raddr, *rreg, *rpal, *fb;
	int index;

	*ADDR_NDP_CSR(sp) = NDCSR_RESET860;	/* Halt the processor. */
	
	raddr = ADDR_DAC_ADDR_PORT(sp);	// Set address
	rreg =  ADDR_DAC_REG_PORT(sp);	// Set reg at current address
	rpal =  ADDR_DAC_PALETTE_PORT(sp);	// Set palette w/ auto-incr
	fb =	ADDR_FRAMESTORE(sp);
	
	// Load up the 3 DACs with blink off, unmasked.
	*raddr = 0x04040404; *rreg = 0xffffff00;
	*raddr = 0x05050505; *rreg = 0;
	*raddr = 0x06060606; *rreg = 0x40404000;
	*raddr = 0x07070707; *rreg = 0;
	
	// Load a standard palette, gamma 2.3
	*raddr = 0x00000000;	// Select start of palatte
	for (index = 0; index < 256; index++)
	{
	    *rpal =   (ND_GammaTable[index] << 24)
	    	    | (ND_GammaTable[index] << 16)
		    | (ND_GammaTable[index] << 8);
	}	
	
	// Clear the screen to a nice 33% grey 
	for ( index = 0; index < ((SIZE_X * SIZE_Y) / 8); ++index )
	{
		*fb++ = 0x55555500;	/* 33% grey! */
		*fb++ = 0x55555500;	/* 33% grey! */
		*fb++ = 0x55555500;	/* 33% grey! */
		*fb++ = 0x55555500;	/* 33% grey! */
		*fb++ = 0x55555500;	/* 33% grey! */
		*fb++ = 0x55555500;	/* 33% grey! */
		*fb++ = 0x55555500;	/* 33% grey! */
		*fb++ = 0x55555500;	/* 33% grey! */
	}
	ND_init_message_system( sp );
}
#else /* ! PROTOTYPE */
void ND_init_hardware (
		ND_var_t *sp )
{
	volatile unsigned char * raddr = (volatile unsigned char *)sp->bt_addr;
	volatile int *fb = ADDR_FRAMESTORE(sp);
	volatile int *csr = (volatile int *)((unsigned)sp->csr_addr + 0x2000);
	unsigned int index;

	*ADDR_NDP_CSR(sp) = NDCSR_RESET860;	/* Halt the processor. */
	*ADDR_NDP_CSR1(sp) = 0;			/* Clear host interrupt. */
	
	// Load up command register 0, 0x201
	raddr[0] = 0x01;
	raddr[4] = 0x02;
		raddr[8] = 0x40;	// 1:1 interleave
	// Command register 1, 0x202
	raddr[0] = 0x02;
	raddr[4] = 0x02;
		raddr[8] = 0x00;	// ??
	// Command register 2, 0x203
	raddr[0] = 0x03;
	raddr[4] = 0x02;
		raddr[8] = 0x80;	// Enable sync, no pedestal
	// Pixel read mask, 0x205-0x208
	raddr[0] = 0x05;
	raddr[4] = 0x02;
		raddr[8] = 0xFF;	// All bits enabled
	raddr[0] = 0x06;
	raddr[4] = 0x02;
		raddr[8] = 0xFF;	// All bits enabled
	raddr[0] = 0x07;
	raddr[4] = 0x02;
		raddr[8] = 0xFF;	// All bits enabled
	raddr[0] = 0x08;
	raddr[4] = 0x02;
		raddr[8] = 0xFF;	// All bits enabled

	// Blink mask, 0x209-0x20c
	raddr[0] = 0x09;
	raddr[4] = 0x02;
		raddr[8] = 0x0;	// All bits disabled
	raddr[0] = 0x0a;
	raddr[4] = 0x02;
		raddr[8] = 0x0;	// All bits disabled
	raddr[0] = 0x0b;
	raddr[4] = 0x02;
		raddr[8] = 0x0;	// All bits disabled
	raddr[0] = 0x0c;
	raddr[4] = 0x02;
		raddr[8] = 0x0;	// All bits disabled

	// Tag table 0x300 - 0x30F. 24 bit word loaded w 3 writes
	for ( index = 0x300; index <= 0x30F; ++index )
	{
		raddr[0] = index & 0xFF;
		raddr[4] = (index & 0xFF00) >> 8;
			raddr[8] = 0x0;
			raddr[8] = 0x1;
			raddr[8] = 0x0;
	}
	
	// Load a ramp in the palatte. First 256 loactions
	for (index = 0; index < 256; index++)
	{
		raddr[0] = index & 0xFF;
		raddr[4] = (index & 0xFF00) >> 8;
		    raddr[0xC] = ND_GammaTable[index];
		    raddr[0xC] = ND_GammaTable[index];
		    raddr[0xC] = ND_GammaTable[index];
	}	
	// Load a ramp in the palatte. Second 256 loactions
	for (index = 0x100; index < 0x200; index++)
	{
		raddr[0] = index & 0xFF;
		raddr[4] = (index & 0xFF00) >> 8;
		    raddr[0xC] = ND_GammaTable[index - 0x100];
		    raddr[0xC] = ND_GammaTable[index - 0x100];
		    raddr[0xC] = ND_GammaTable[index - 0x100];
	}	
	// Last 4 locations 0x20C to 0x20F must be FF 
	for (index = 0x20C; index < 0x210; index++)
	{
		raddr[0] = index & 0xFF;
		raddr[4] = (index & 0xFF00) >> 8;
		    raddr[0xC] = 0xFF;
		    raddr[0xC] = 0xFF;
		    raddr[0xC] = 0xFF;
	}
#if ! defined(CURSOR_i860)
	if ( fb != (volatile int *) 0 )
	{
	    // Clear the screen to a nice 33% grey 
	    for ( index = 0; index < ((SIZE_X * SIZE_Y) / 8); ++index )
	    {
		*fb++ = 0x55555500;	/* 33% grey! */
		*fb++ = 0x55555500;	/* 33% grey! */
		*fb++ = 0x55555500;	/* 33% grey! */
		*fb++ = 0x55555500;	/* 33% grey! */
		*fb++ = 0x55555500;	/* 33% grey! */
		*fb++ = 0x55555500;	/* 33% grey! */
		*fb++ = 0x55555500;	/* 33% grey! */
		*fb++ = 0x55555500;	/* 33% grey! */
	    }
	    /* Clear out the rest of the frame buffer. */
	    while ( fb < (volatile int *)(((unsigned)ADDR_FRAMESTORE(sp)) + ND_FB_SIZE) )
		    *fb++ = 0;
	}
#endif
	// Unblank the screen by poking the memory controller
	*csr = 0x00000001;
}
#endif /* ! PROTOTYPE */

