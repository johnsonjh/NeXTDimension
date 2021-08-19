//
//	NextDimension Boot ROM Code.
//
#include "i860/trap.h"
#include "i860/psl.h"
#include "i860/reg.h"

#define NOP6	nop;nop;nop;nop;nop;nop
	base_addr = 0x08000000	// Base of DRAM.  The Slot gets ORed in.
	FSR_BITS = 1		// Flush zero, no traps, round nearest

#define NDCSR_CS8		0x00000002
#define NDCSR_INT860ENABLE	0x00000004
#define NDCSR_KEN860		0x00001000
#define NDCSR_BE_IEN		0x00000010

	ADDR_MASK = 0x0FFFFFFF	// mask out high nibble, for slot address

// Slot dependent addresses
	DRAM_BASE = 0x08000000	// Slot gets ORed into high nibble later
	VRAM_BASE = 0x0E000000	// Slot gets ORed into high nibble later
	DITHER_RAM_TOP = 0xFF000200	// Top of Dither RAM in datapath chip

// Slot independent addresses
	ND_SLOTID = 0xFF800030	// Address of MC reg holding our Slot ID
	ND_CSR0	=   0xFF800000	// Address of MC CSR 0
	
	.text
CheckSum:		// The first 4 bytes in the ROM hold the checksum
	.long 0

// ROM should be linked -T FFFF0000.
Reset::
	nop				// Cached, start of 1st line
	nop				// 2 NOPs on reset for chip bug

	// turn off all interrupts
	ld.c	psr,r16
	andnot	PSR_IM,r16,r16
	st.c	r16,psr
	
	// Flip on big-endian data mode,
	// allow write protect of kernel text pages.
	.align 32		// CHIP BUG Step B2, Bug 25
	ixfr	r0,f0
	ld.c	epsr,r16
	orh	h%(EPSR_BE+EPSR_WP),r16,r16
	or	l%(EPSR_BE+EPSR_WP),r16,r16
	st.c	r16,epsr

	// Perform post-reset conditioning, initialization
	//
	//	These operations are largely 'magic', intended to put the i860
	//	into a sane state, so we get consistant reasonable behavior
	//
	ld.c	fir, r0		// Reset FIR to work in trap handler

	or	DIR_ITI+DIR_CS8,r0,r16	// Set ITI to nuke insn and AT caches

	.align 32	// Changing DTB and ITI, so meet all the constraints...
	st.c	r16,dirbase	// 4 Kb pages, default cache, no ATE yet
	NOP6

	// Init FPU pipelines
	orh	h%FSR_BITS,r0,r16
	or	l%FSR_BITS,r16,r16
	st.c	r16, fsr
	r2apt.ss	f0,f0,f0	// Init Mult and Add pipelines, and
	r2apt.ss	f0,f0,f0	// init KR and T registers
	r2apt.ss	f0,f0,f0
	i2apt.ss	f0,f0,f0	// Switch and init the KI register
	
	pfiadd.ss	f0,f0,f0	// Init graphics pipeline
	pfiadd.ss	f0,f0,f0
	
	st.c	r16, fsr		// Clear any error bits flushed out
	
//	Enable the cache, as required by CHIP BUG 43 Workaround
	//	Enable the cache
	orh	ha%ND_CSR0, r0, r16
	or	NDCSR_KEN860+NDCSR_CS8+NDCSR_INT860ENABLE, r0, r17
	st.l	r17, l%ND_CSR0(r16)

//	Set up the cache tag bits.
	call	_init_data_cache
	nop
// At this point, the processor should be fully initialized
	//	Disable the cache
	orh	ha%ND_CSR0, r0, r16
	or	NDCSR_CS8+NDCSR_INT860ENABLE, r0, r17
	st.l	r17, l%ND_CSR0(r16)

//
//	Establish a stack and run the initialization code, written in C.
//
//	The initial stack is set in the 512 bytes of dither RAM in the 
//	datapath chip.  While this is in use, the cache MUST be off.  Only
//	int sized transactions will work to this RAM.
//
	orh	h%DITHER_RAM_TOP,r0,sp
	or	l%DITHER_RAM_TOP,sp,sp
	adds	-16,sp,sp
	adds	8,sp,fp			// Build bogus stack frame
	st.l	fp,8(sp)
	st.l	r0,12(sp)		// Bogus return address, for debugger

	call	_early_start		// Init HW, config memory.
	nop				// Return a stack pointer in r16
//
//	_early_start has configured memory, set up out globals area,
//	and returned a suitable stack pointer in r16.
//
//	It's safe to use the cache now.
//
	adds	-16,r16,sp		// Set up a new stack in DRAM
	adds	8,sp,fp			// Build bogus stack frame
	st.l	fp,8(sp)
	st.l	r0,12(sp)
	call	_main
	nop
//
//	On return from main(), fall through to the transfer code.
//

//	Spin waiting for a host interrupt to reach us.  We look for the
//	interrupt in the EPSR, so as to avoid making bus cycles.
_TransferToKernel:	.globl _TransferToKernel
	//	Enable the cache
	orh	ha%ND_CSR0, r0, r16
	or	NDCSR_KEN860+NDCSR_CS8+NDCSR_INT860ENABLE, r0, r17
	st.l	r17, l%ND_CSR0(r16)
	
	call	_flush
	nop

	//	Mask for i860 interrupt bit in CSR, into r3
	or	l%EPSR_INT, r0, r3
	orh	h%EPSR_INT, r3, r3

	//	Slot ID for bits 28-31 of DRAM addr, in r4
	orh	ha%ND_SLOTID, r0, r16
	ld.l	l%ND_SLOTID(r16), r4
	shl	28, r4, r4		// r4 holds high nybble of address

wait_here:
	ld.c	epsr, r2
	and	r2, r3, r0	// CC set if 0, or no interrupt
	bc.t	wait_here	// Branch if CC set
	nop
	
	//	The transfer address is in r1, and the dirbase value to
	//	use is in r2.
	//	We run through the transfer code twice, to get the
	//	code into the cache.  We MUST be executing from the
	//	cache when we turn off CS8 mode, or we may not make it
	//	to DRAM.
	//
	orh	h%setup_xfr, r0, r1
	or	l%setup_xfr, r1, r1
	ld.c	dirbase, r2
	nop

transfer:
	.align 16			// NOPs til we get to a cache line.
	nop				// Last cache line!
	st.c	r2, dirbase
	bri	r1
	NOP6
	
setup_xfr:
	orh	h%base_addr, r4, r1	// Load up 32 bit branch dst addr
	or	l%base_addr, r1, r1
	or	DIR_CS8, r0, r16	// Set up to kill CS8
	andnot	r16, r2, r2		// kill CS8
	//	Disable the cache
	orh	ha%ND_CSR0, r0, r16
	or	NDCSR_CS8+NDCSR_INT860ENABLE, r0, r17
	st.l	r17, l%ND_CSR0(r16)
	br	transfer
	nop
	
//
//	SERVICE SUBROUTINES FOR BOOT ROM
//

//
// cache enable and disable routines.
//
_cache_on:	.globl	_cache_on
	//	Enable the cache
	orh	ha%ND_CSR0, r0, r17
	or	NDCSR_KEN860+NDCSR_CS8+NDCSR_INT860ENABLE, r0, r16
	bri	r1			// Return after next insn
	st.l	r16, l%ND_CSR0(r17)
	
_cache_off:	.globl	_cache_off
	//	Enable the cache
	orh	ha%ND_CSR0, r0, r17
	or	NDCSR_CS8+NDCSR_INT860ENABLE, r0, r16
	bri	r1			// Return after next insn
	st.l	r16, l%ND_CSR0(r17)
	
//
// abs_addr():
//
//	Given a kernel virtual address, return the correct absolute address
//	to be used.  This is an identity function if VM is up and running.
//
//	The address passed in in r16 is returned, modified.
//	Registers r17 and r18 are trashed.
//
_abs_addr:	.globl	_abs_addr
	ld.c	dirbase,r17
	and	1,r17,r0
	bnc	1f		// VM is running.  Don't munge addresses
	
	// Set up address twiddling registers
	orh	ha%ND_SLOTID, r0, r31
	ld.l	l%ND_SLOTID(r31), r18
	
	orh	h%ADDR_MASK, r0, r17
	or	l%ADDR_MASK, r17, r17

	and	r16, r17, r16	// Mask out high nibble
	and	0x7, r18, r18	// Use only Slot ID bits.
	shl	28, r18, r18	// Move slot # to high nibble
	or	r16, r18, r16	// OR in the slot number for the high nibble
1:
	bri	r1
	nop	

