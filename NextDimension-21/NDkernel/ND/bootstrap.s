#include "i860/trap.h"
#include "i860/psl.h"
#include "i860/reg.h"

#define NOP6	nop;nop;nop;nop;nop;nop

	.data
	.globl intstack		// Starting top of interrupt stack, process 0
	.align 4 
SingleZero:	.float  0.0

	.text
//
//	Extern global text declarations
//
	.globl	_init_data_cache //  in flush.s
	.globl	alltraps	// Trap handler vector, in locore.s
	.globl	_vstart		// VM startup code, in locore.s
	FSR_BITS = 1		// Flush zero, no traps, round nearest

#if defined(PROTOTYPE)
	// The dawn of time... No stack, and no initialization other than RESET.
_start:	.globl _start

	// turn off all interrupts
	ld.c	psr,r16
	andnot	PSR_IM,r16,r16
	st.c	r16,psr
	
	// Flip on big-endian data mode, allow write protect of kernel text pages.
	.align 32		// CHIP BUG Step B2, Bug 25
	ixfr	r0,f0
	ld.c	epsr,r16
	orh	h%(EPSR_BE+EPSR_WP),r16,r16
	or	l%(EPSR_BE+EPSR_WP),r16,r16
	st.c	r16,epsr

	
	// setup boot stack
	orh	h%intstack,r0,sp
	or	l%intstack,sp,sp
	adds	-16,sp,sp
	adds	8,sp,fp			// Build bogus stack frame
	st.l	fp,8(sp)
	orh	h%_vstart,r0,r16	// and return address
	or	l%_vstart,r16,r16
	st.l	r16,12(sp)
	

	// Perform post-reset conditioning, initialization
	//
	//	These operations are largely 'magic', intended to put the i860
	//	into a sane state, so we get consistant reasonable behavior
	//
	ld.c	fir, r0		// Reset FIR to work in trap handler

	or	DIR_ITI,r0,r16	// Set ITI to nuke instruction and AT caches

	.align 32		// Changing DTB and ITI, so meet all the constraints...
	st.c	r16,dirbase	// 4 Kb pages, default cache, no ATE yet
	NOP6
	
	
	// Flush the data cache
	call	_init_data_cache		// in flush.s
	nop

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
	
	orh	ha%SingleZero,r0,r17	// Init the FP load pipeline
	pfld.l	l%SingleZero(r17),f0
	pfld.l	l%SingleZero(r17),f0
	pfld.l	l%SingleZero(r17),f0
	
	st.c	r16, fsr		// Clear any error bits flushed out
	
	// At this point, the processor should be fully initialized

	// grab the trap vector
	orh	0xffff,r0,r20
	or	0xff00,r20,r20		// Hard vector
	orh	h%alltraps,r0,r21
	or	l%alltraps,r21,r21	// Vector in .text
	subu	r21,r20,r22		// Compute relative address
	shr	2,r22,r22
	adds	-3,r22,r22		// adust for 2 NOPS, +1
					// change if remove NOPS!!!!
	xor	1,r22,r22		// Convert to little-endian text address
	
	andnoth 0xfc00,r22,r22
	orh	0x6800,r22,r22		// br alltraps
	orh	0xa000,r0,r23		// nop

	st.l	r23,0(r20)		// chip bug NOP
	st.l	r23,4(r20)		// chip bug NOP
	st.l	r22,8(r20)		// br alltraps
	st.l	r23,12(r20)		// delayed NOP
	st.l	r23,16(r20)		// delayed NOP
	st.l	r23,20(r20)		// delayed NOP
	st.l	r23,24(r20)		// delayed NOP
	st.l	r23,28(r20)		// delayed NOP

	br	_vstart			// Complete the VM setup back in locore.s
	nop


#else	/* Not PROTOTYPE */

#define	AddrMask	r5
#define SlotAddr	r6

	ADDR_MASK = 0x0FFFFFFF	// mask out high nibble, for slot address

// Slot dependent addresses
	DRAM_BASE = 0x08000000	// Slot gets ORed into high nibble later

// Slot independent addresses
	ND_SLOTID = 0xFF800030	// Address of MC reg holding our Slot ID
	ND_CSR0	=   0xFF800000	// Address of MC CSR 0
// Page table stuff
	PDESHIFT =  22
	PDEMASK	=   0x3ff

//
//	Interrupt/context 0 stack and whatnot.  Note that all addresses are
//	invalid until we get the MMU on line!
//
//	Before using any address, mask out the high nibble and or in the slot ID
//	bits.  This will produce a correct physical address.
//
	.data
	.globl _bad		// Trashable 8K band for flush(), bootflush()
//
//	Bootstrap page tables, as defined in lodata.o
//
	.globl kpde		// Kernel page directory, to be loaded into dirbase
	.globl __start_page_tables_	// Start of pages holding page tables
	.globl __end_page_tables_	// End of pages holding page tables

	.text
//
//	Extern global text declarations
//
	.globl	alltraps	// Trap handler vector, in locore.s
//
//	The dawn of time... No stack, and no initialization other than RESET.
//	The first insn in locore.s branches here to get things rolling.
//
//	This code is called with and uses only PC relative branches.  This code
//	is run before the MMU is active, and as a consequence all addresses are
//	incorrect in their high nibble!
//
//	Before using any address, mask out the high nibble and or in the slot ID
//	bits.  This will produce a correct physical address.
//
_start:	.globl _start
	
	orh	ha%0x28100000,r0,r16		// TRACE
	st.l	r0,l%0x28100000(r16)		// TRACE

	// turn off all interrupts
	ld.c	psr,r16
	andnot	PSR_IM,r16,r16
	st.c	r16,psr
	
	// Flip on big-endian data mode, allow write protect of kernel text pages.
	.align 32		// CHIP BUG Step B2, Bug 25, for 'ld.c epsr, reg'
	ixfr	r0,f0
	ld.c	epsr,r16
	orh	h%(EPSR_BE+EPSR_WP),r16,r16
	or	l%(EPSR_BE+EPSR_WP),r16,r16
	st.c	r16,epsr
	nop

	// Perform post-reset conditioning, initialization
	//
	//	These operations are largely 'magic', intended to put the i860
	//	into a sane state, so we get consistant reasonable behavior
	//
	//	Eventually most of this should migrate into the boot ROM.
	//
	ld.c	fir, r0		// Reset FIR to work in trap handler

	or	DIR_ITI,r0,r16	// Set ITI to nuke instruction and AT caches

	.align 32		// Changing DTB and ITI, so meet all the constraints...
	st.c	r16,dirbase	// 4 Kb pages, default cache, no ATE yet
	NOP6
	
	// Inhibit interrupts and the cache by clearing the MC CSR 0
	or	0x10,r0,r17		// BE enable
	orh	ha%ND_CSR0, r0, r16
	st.l	r17, l%ND_CSR0(r16)		// I hope this works...

	// Set up address twiddling registers
	orh	h%ADDR_MASK, r0, AddrMask
	or	l%ADDR_MASK, AddrMask, AddrMask

	orh	ha%ND_SLOTID, r0, r16
	ld.l	l%ND_SLOTID(r16), SlotAddr	// I hope this works...
	shl	28, SlotAddr, SlotAddr
	
	
	// Flush the data cache.  Gawd only knows why, but the i860 VM
	// won't work right unless we do this right after a RESET.
	call	bootflush
	nop
	or	1,r0,r17			// TRACE
	orh	ha%0x28100000,r0,r16		// TRACE
	st.l	r17,l%0x28100000(r16)		// TRACE


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
	
	orh	h%SingleZero,r0,r17	// Init the FP load pipeline
	or	l%SingleZero, r17, r17
	and	AddrMask, r17, r17	// Zap the high nibble
	or	SlotAddr, r17, r17	// Convert to physical address!
	pfld.l	0(r17),f0
	pfld.l	0(r17),f0
	pfld.l	0(r17),f0
	
	st.c	r16, fsr		// Clear any error bits flushed out

	//
	// Fill in globals used to manage slot and virt address stuff
	//
	orh	h%__slot_id_,r0,r16
	or	l%__slot_id_, r16, r16
	and	AddrMask, r16, r16	// Zap the high nibble
	or	SlotAddr, r16, r16	// Convert to physical address!
	st.l	SlotAddr, 0(r16)
	
	or	2,r0,r17			// TRACE
	orh	ha%0x28100000,r0,r16		// TRACE
	st.l	r17,l%0x28100000(r16)		// TRACE

//
//	Fill in our prebuilt page tables with the slot ID
//
	or	l%__start_page_tables_, r0, r17
	orh	h%__start_page_tables_, r17, r17
	and	AddrMask, r17, r17	// Convert to physical address
	or	SlotAddr, r17, r17	// by putting Slot # in high nibble
	orh	h%__end_page_tables_, r0, r18
	or	l%__end_page_tables_, r18, r18
	and	AddrMask, r18, r18	// Convert to physical address
	or	SlotAddr, r18, r18	// by putting Slot # in high nibble
Loop:
	ld.l	0(r17), r16		// Fetch a page table entry
	bte	r16, r0, SkipZeroes	// If it's zero, skip it
	and	AddrMask, r16, r16	// Convert to physical address
	or	SlotAddr, r16, r16	// by putting Slot # in high nibble
	st.l	r16, 0(r17)		// and put it back....
SkipZeroes:
	addu	4, r17, r17
	subu	r17, r18, r0		// At end yet?
	bnc	Loop
//
//	Map DRAM onto itself (a 1:1 or transparent mapping) so we can survive
//	the transfer to VM addressing.
//
	or	3,r0,r17			// TRACE
	orh	ha%0x28100000,r0,r16		// TRACE
	st.l	r17,l%0x28100000(r16)		// TRACE

	orh	h%DRAM_BASE, SlotAddr, r22	// Phys address of base of DRAM
	shr	PDESHIFT, r22, r22		// Convert address to a page
	and	PDEMASK, r22, r22		// directory index.
	xor	1, r22, r22			// Toggle to big-endian format
	shl	2, r22, r22			// Scale for use as a 32 bit index
	orh	h%kpde, r0, r23			// Fetch address of page directory
	or	l%kpde, r23, r23		
	and	AddrMask, r23, r23		// Convert to physical address
	or	SlotAddr, r23, r23		// by putting Slot # in high nibble
	orh	h%transparent_DRAM, r0, r18	// Fetch address of page table to
	or	l%transparent_DRAM, r18, r18	// load into the directory
	and	AddrMask, r18, r18		// Convert to physical address
	or	SlotAddr, r18, r18		// by putting Slot # in high nibble
	addu	r22, r23, r24			// Compute target address
	st.l	r18, 0(r24)			// and stuff it in the directory

//
//	Transfer to VM addressing.  r23 holds phys addr of kpde from above.
//
	ld.c	dirbase,r16
	and	0xfff,r16,r16		// Save any neat bits set in dirbase
	or	DIR_ITI+DIR_ATE,r16,r16	// Invalidate TLB, enable address translation
	andnot	0xfff,r23,r23		// These bits SHOULD be zero already...
	or	r23,r16,r16		// Fold in saved neat bits and addr.

	or	4,r0,r30			// TRACE
	orh	ha%0x28100000,r0,r31		// TRACE
	st.l	r30,l%0x28100000(r31)		// TRACE
	st.l	r16,l%0x28100004(r31)		// TRACE

	.align 32		// Changing ITI and DTB, so meet all the constraints...
	st.c	r16,dirbase		// And switch to virtual addressing.
	NOP6

	or	5,r0,r17			// TRACE
	orh	ha%0xf8100000,r0,r16		// TRACE
	st.l	r17,l%0xf8100000(r16)		// TRACE

//
//	Jump to the virtual address that we should continue execution at...
//	Honest, this really has to be done.  Physical addresses don't match
//	virtual addresses, once we destroy the transparent mapping (It's in the way
//	of the WindowServer i860 code!).
//
	orh	h%gone_virtual,r0,r20	// Load the virtual (linked) address
	or	l%gone_virtual,r20,r20	// we will continue at.
	nop	
	bri	r20			// Transfer from 1:1 mapped to virtual addr
	nop

gone_virtual:
//
//	Get rid of that 1:1 DRAM mapping that we set up before turning VM on
//
	or	6,r0,r17			// TRACE
	orh	ha%0x28100000,r0,r16		// TRACE
	st.l	r17,l%0x28100000(r16)		// TRACE

	orh	h%kpde, r0, r23			// Fetch VIRTUAL address
	or	l%kpde, r23, r23		// of page directory.  The index in
						// r22 is still valid.
	addu	r22, r23, r24			// Compute target address
	st.l	r0, 0(r24)			// Clear the 1:1 directory entry
	ld.c	dirbase, r16
	or	DIR_ITI,r16,r16	// Set ITI to nuke instruction and AT caches

	.align 32		// Changing ITI, so meet all the constraints...
	st.c	r16,dirbase
	NOP6
//
//	At this point, the processor should be fully initialized,
//	and DRAM mapped in to replace the ROM code at the RESET vector.
//
//	All addresses should now be correct!
//
//	Rewrite the RESET vector to point to our trap handler.
//

	// grab the trap vector
	orh	0xffff,r0,r20
	or	0xff00,r20,r20		// Hard vector
	orh	h%alltraps,r0,r21
	or	l%alltraps,r21,r21	// Vector in .text, locore.s
	subu	r21,r20,r22		// Compute relative address
	shr	2,r22,r22
	adds	-3,r22,r22		// adust for 2 NOPS, +1
					// change if remove NOPS!!!!
	xor	1,r22,r22		// Convert to little-endian text address
	
	andnoth 0xfc00,r22,r22
	orh	0x6800,r22,r22		// br alltraps
	orh	0xa000,r0,r23		// nop

	st.l	r23,0(r20)		// chip bug NOP
	st.l	r23,4(r20)		// chip bug NOP
	st.l	r22,8(r20)		// br alltraps
	st.l	r23,12(r20)		// delayed NOP
	st.l	r23,16(r20)		// delayed NOP
	st.l	r23,20(r20)		// delayed NOP
	st.l	r23,24(r20)		// delayed NOP
	st.l	r23,28(r20)		// delayed NOP

	// setup boot stack
	orh	h%intstack,r0,sp
	or	l%intstack,sp,sp
	adds	-16,sp,sp
	adds	8,sp,fp			// Build bogus stack frame
	st.l	fp,8(sp)
	orh	h%_vstart,r0,r16	// and return address
	or	l%_vstart,r16,r16
	st.l	r16,12(sp)

	br	_vstart			// Complete the VM setup back in locore.s
	nop

	.text
//
// abs_addr():
//
//	Given a kernel virtual address, return the correct absolute address
//	to be used.  This is an identity function if VM is up and running.
//
_abs_addr:	.globl	_abs_addr
	ld.c	dirbase,r17
	and	1,r17,r0
	bnc	1f		// VM is running.  Don't munge addresses
	
	// Set up address twiddling registers
	orh	h%ADDR_MASK, r0, r17
	or	l%ADDR_MASK, r17, r17

	orh	ha%ND_SLOTID, r0, r31
	ld.l	l%ND_SLOTID(r31), r18
	shl	28, r18, r18
	
	and	r16, r17, r16	// Mask out high nibble
	or	r18, r16, r16	// OR in the slot number for the high nibble
1:
	bri	r1
	nop	


//
// bootflush():
//
//	Flush the contents of the cache to memory.
//	See Intel i860 PRM page 5-15, example 5-2 for 'details'
//
//	I don't know why it works.  It just does...
//
//	NOTE:  Other code assumes that r28, r29, and r30 are untouched here.
//
//	CHIP BUG: Step B2, Bug 23.  Code in this module assumes that it is aligned
//		on a 32 byte boundry. Bletch
//
//	This version differs from the standard flush in flush.s, in that it
//	computes the correct physical address for the _bad flush buffer, lumping
//	in the slot_id in the high nibble of the address.
//
//	It also complies with the workarounds for CHIP BUG 43, for C0 and earlier
//	step i860 chips.
//	
//
#define Rw		r16
#define Rx		r17
#define Ry		r18
#define Rz		r19
#define R_save_r1	r20
#define R_save_psr	r21
#define R_save_fsr	r22
#define NDCSR_KEN860		0x00001000

//
//
//	bootflush()
//
bootflush:
	mov	r1, R_save_r1
//	Disable traps and interrupts during init.
	ld.c	psr, R_save_psr
	andnot	PSR_IM,R_save_psr,Rz
	st.c	Rz, psr
	nop
	ld.c	fsr, R_save_fsr
	or	FSR_BITS,r0,Rz
	st.c	Rz, fsr
//	Enable the cache, as required by CHIP BUG 43 Workaround
	orh	ha%ND_CSR0, r0, r16
	or	NDCSR_KEN860, r0, r17
	st.l	r17, l%ND_CSR0(r16)		// I hope this works...
//	The init code, from the Intel recommended workaround.
	ld.c	dirbase,	Rz
	or	0x800,	Rz,	Rz	// RC <-- 0b10 (assuming was 00)
	adds	-1,	r0,	Rx	// Rx <-- -1 (loop increment)
	orh	h%_bad-32, r0,	Rw	// Rw gets address minus 32 of flush area
	or	l%_bad-32, Rw,	Rw
	and	AddrMask, Rw, Rw	// Zap the high nibble
	or	SlotAddr, Rw, Rw	// Convert to physical address!
	call	D_INIT
	st.c	Rz,	dirbase		// Replace in block 0
	or	0x900,	Rz,	Rz	// RB <-- 0b01
	call	D_INIT
	st.c	Rz,	dirbase
	xor	0x900,	Rz,	Rz	// Clear RC and RB
	st.c	Rz,	dirbase
	NOP6
// End of cribbed code.
//	Disable the cache
	orh	ha%ND_CSR0, r0, r16
	st.l	r0, l%ND_CSR0(r16)		// I hope this works...
//	Restore traps and interrupts
	st.c	R_save_fsr, fsr
	st.c	R_save_psr, psr
	nop
	bri	R_save_r1
	nop
D_INIT:
	or	127,	r0,	Ry		// Ry <-- loop count
	bla	Rx,	Ry,	D_INIT_LOOP	// One time to initialize LCC
	nop
D_INIT_LOOP:
	bla	Rx,	Ry,	D_INIT_LOOP	// Loop; execute next instruction for
						// for 128 lines in cache block
	    flush	32(Rw)++		// Init and increment to next line
						// No writebacks guaranteed after reset
	bri	r1				// return after next instruction
	nop
#endif	/* Not PROTOTYPE */
