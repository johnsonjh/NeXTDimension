#include "i860/trap.h"
#include "i860/psl.h"
#include "i860/reg.h"

#define CHIP_BUG_43	1

#define NOP6	nop;nop;nop;nop;nop;nop
	.data
	.globl _bad		// 8 Kb trashable range of memory

	.text
	FSR_BITS = 1		// Flush zero, no traps, round nearest
//
// flush():
//
//	Flush the contents of the cache to memory.
//	See Intel i860 PRM page 5-15, example 5-2 for 'details'
//
//	I don't know why it works.  It just does...
//
//	NOTE:  Other code assumes that r28, r29, and r30 are untouched here.
//
//	CHIP BUG: Step B2, Bug 23.  Code in this module assumes that it is aligned
//		on a 32 bit boundry.  Therefore, rather than being compiled
//		independently, this module is #included into locore.s. Bletch
//
#define Rw		r16
#define Rx		r17
#define Ry		r18
#define Rz		r19
#define R_save_r1	r20
#define R_save_psr	r29
#define R_save_fsr	r30

#if !defined(CHIP_BUG_43)

//
// init_data_cache():
//	Do whatever magic is needed to initialize the data cache.
//	This must be called before using the data cache.  On C-1 and
//	later steppings of the chip, we just run the flush() routine.
//
_init_data_cache:	.globl _init_data_cache
	br _flush
	nop
//
//	flush data cache and invalidate tlb and instr cache
	.globl	_flush_inv
_flush_inv:
	mov	r1, R_save_r1
//	Disable traps and interrupts during flush
	ld.c	psr, R_save_psr
	andnot	PSR_IM,R_save_psr,Rz
	st.c	Rz, psr
	nop
	ld.c	fsr, R_save_fsr
	or	FSR_BITS,r0,Rz
	st.c	Rz, fsr
//	The flush code, from the book.
	ld.c	dirbase,	Rz
	or	0x800,	Rz,	Rz	// RC <-- 0b10 (assuming was 00)
	adds	-1,	r0,	Rx	// Rx <-- -1 (loop increment)
	call	D_FLUSH
	st.c	Rz,	dirbase		// Replace in block 0
	or	0x900,	Rz,	Rz	// RB <-- 0b01
	call	D_FLUSH
	st.c	Rz,	dirbase
	xor	0x900,	Rz,	Rz	// Clear RC and RB
// Set ITI, invalidating the TLB and the instruction cache.
	or	DIR_ITI,Rz,Rz
	.align 32		// Changing ITI, so meet all the constraints...
	ixfr	r0, f0	// CHIP BUG #48 Workaround. FP traps are disabled.
	st.c	Rz,	dirbase
	NOP6
// End of cribbed code.
//	Restore traps and interrupts
	st.c	R_save_fsr, fsr
	st.c	R_save_psr, psr
	nop
	bri	R_save_r1
	nop
	
//
//	flush()
//
_flush:	.globl _flush
	mov	r1, R_save_r1
//	Disable traps and interrupts during flush
	ld.c	psr, R_save_psr
	andnot	PSR_IM,R_save_psr,Rz
	st.c	Rz, psr
	nop
	ld.c	fsr, R_save_fsr
	or	FSR_BITS,r0,Rz
	st.c	Rz, fsr
//	The flush code, from the book.
	ld.c	dirbase,	Rz
	or	0x800,	Rz,	Rz	// RC <-- 0b10 (assuming was 00)
	adds	-1,	r0,	Rx	// Rx <-- -1 (loop increment)
	call	D_FLUSH
	st.c	Rz,	dirbase		// Replace in block 0
	or	0x900,	Rz,	Rz	// RB <-- 0b01
	call	D_FLUSH
	st.c	Rz,	dirbase
	xor	0x900,	Rz,	Rz	// Clear RC and RB
//	Change DTB, ATE, or ITI fields here, if necessary.
	st.c	Rz,	dirbase
//	End of cribbed code.
//	Restore traps and interrupts
	st.c	R_save_fsr, fsr
	st.c	R_save_psr, psr
	nop
	bri	R_save_r1
	nop
	
D_FLUSH:
	orh	h%(_bad-32), r0, Rw	// Rw <-- address - 32
	or	l%(_bad-32), Rw, Rw	//  of flush area.
	or	127,	r0,	Ry	// Ry <-- loop count
	ld.l	32(Rw),	r31		// CHIP BUG: Step B2, Bug 23
	ld.l	32(Rw),	r31		// Clear any pending bus writes
	shl	0,	r31,	r31	// Wait until load finishes
	bla	Rx, Ry, D_FLUSH_LOOP	// One time to initialize LCC
	nop
	.align 32			// CHIP BUG: Step B2, Bug 23
D_FLUSH_LOOP:
	ixfr	r0,	f0		// CHIP BUG: Step B2, Bug 23
	bla	Rx, Ry, D_FLUSH_LOOP	// Loop: execute next insn for 128
	    flush	32(Rw)++	// lines in cache block
	ixfr	r0,f0			// CHIP BUG Step B2, C0, Mystery Bug
					// (Intel says add this)
	bri	r1			// Return after next insn
	ld.l	-512(Rw),	r0	// Load from flush area to clear pending
					// writes.  A hit is guaranteed.
#else	/* CHIP_BUG_43 */

//	CHIP BUG 43:	flush opcode with HOLD asserted may corrupt data or hang
//			the processor.	Steps C0 and earlier...
//
//	In systems using HOLD, the bus cycle for a writeback caused by a FLUSH
//	instruction can be "lost", causing the writeback data to be stuck in the
//	internal write buffers.  This condition will cause data corruption and
//	may cause the i860 to hang.
//
//	The problem occurs only when both of the following two conditions are met:
//
//	a)	The data cache has at least two cache line entries which have been
//		half modified.  (EITHER the upper or lower half is modified.)
//	b)	HOLD occurs within the FLUSH routine!
//
//	The following code is Intel's recommended workaround.  The D-cache flush()
//	and initialization routines require an 8 Kbyte RESERVED CACHEABLE memory area.
//	The reserved area must be hardware (KEN#) and page table (CD & WT) cacheable.
//
// init_data_cache():
//	Do whatever magic is needed to initialize the data cache.
//	This must be called before using the data cache.  On C-1 and
//	later steppings of the chip, we just run the flush() routine.
//
//
//	flush data cache and invalidate tlb and instr cache
	.globl	_flush_inv
_flush_inv:
//	Disable traps and interrupts during flush
	ld.c	psr, R_save_psr
	andnot	PSR_IM,R_save_psr,r16
	st.c	r16, psr
	nop
	ld.c	fsr, R_save_fsr
	or	FSR_BITS,r0,r16
	st.c	r16, fsr
//	The flush code, from the book.
	ld.c	dirbase, r17	// Load and save dirbase in r17
	adds	-1,r0,r19	// loop decrement
	or	127,r0,r20	// Loop counter
	orh	h%_bad-32, r0,	r16	// r16 gets address minus 32 of flush area
	or	l%_bad-32, r16,	r16

	// First half of cache flush
	andnot	0xF00,r17,r18		// Clear RC, RB, allows entry w any RC,RB
	or	0x800,r18,r18		// Set RC == 2
	bla	r19,r20,flush1		// One time to init LCC
	st.c	r18,dirbase		// Store back dirbase with RC==2, RB==0

flush1:
	bla	r19,r20,flush1		// Loop on next insn 128 times
	    fld.d	32(r16)++,f0	// Load and autoincrement, forcing
	    				// writebacks on modified lines

	// Second half of cache flush, done by changing RB
	or	0x900,r18,r18		// RC==2, RB==1
	or	127, r0, r20		// Reload loop counter
	bla	r19,r20,flush2		// One time to init LCC
	st.c	r18,dirbase		// Store back dirbase with RC==2, RB==1

flush2:
	bla	r19,r20,flush2		// Loop on next insn 128 times
	    fld.d	32(r16)++,f0	// Load and autoincrement, forcing
	    				// writebacks on modified lines

// End of cribbed code.
//
	or	DIR_ITI,r17,r17	// Set up to invalidate TLB
	.align 32		// Changing DTB, so meet all the constraints...
	ixfr	r0, f0		// CHIP BUG #48 Workaround.  FP traps are disabled.
	st.c	r17,dirbase
	NOP6
//	Restore traps and interrupts
	st.c	R_save_fsr, fsr
	st.c	R_save_psr, psr
	bri	r1
	nop
	
//
//	flush()
//
_flush:	.globl _flush
//	Disable traps and interrupts during flush
	ld.c	psr, R_save_psr
	andnot	PSR_IM,R_save_psr,r16
	st.c	r16, psr
	nop
	ld.c	fsr, R_save_fsr
	or	FSR_BITS,r0,r16
	st.c	r16, fsr
//	The flush code, from the book.
	ld.c	dirbase, r17	// Load and save dirbase in r17
	adds	-1,r0,r19	// loop decrement
	or	127,r0,r20	// Loop counter
	orh	h%_bad-32, r0,	r16	// r16 gets address minus 32 of flush area
	or	l%_bad-32, r16,	r16

	// First half of cache flush
	andnot	0xF00,r17,r18		// Clear RC, RB, allows entry w any RC,RB
	or	0x800,r18,r18		// Set RC == 2
	bla	r19,r20,flush3		// One time to init LCC
	st.c	r18,dirbase		// Store back dirbase with RC==2, RB==0

flush3:
	bla	r19,r20,flush3		// Loop on next insn 128 times
	    fld.d	32(r16)++,f0	// Load and autoincrement, forcing
	    				// writebacks on modified lines

	// Second half of cache flush, done by changing RB
	or	0x900,r18,r18		// RC==2, RB==1
	or	127, r0, r20		// Reload loop counter
	bla	r19,r20,flush4		// One time to init LCC
	st.c	r18,dirbase		// Store back dirbase with RC==2, RB==1

flush4:
	bla	r19,r20,flush4		// Loop on next insn 128 times
	    fld.d	32(r16)++,f0	// Load and autoincrement, forcing
	    				// writebacks on modified lines
//
// End of cribbed code.
//
//	Restore traps and interrupts
	st.c	R_save_fsr, fsr
	st.c	R_save_psr, psr
	bri	r1
	    st.c	r17,dirbase
#endif	/* CHIP_BUG_43 */
