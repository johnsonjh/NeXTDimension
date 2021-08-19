#include "i860/trap.h"
#include "i860/psl.h"
#include "i860/reg.h"

#define NOP6	nop;nop;nop;nop;nop;nop

// Defs to try and match the examples in the PRM for pipeline save/restore
#define Mres3	f6
#define Ares3	f8
#define	Mres2	f10
#define Ares2	f12
#define Mres1	f14
#define Ares1	f16
#define Ires1	f18
#define Lres	f20
#define	KR	f22
#define	KI	f24
#define T	f26

#define	Fsr3	r29
#define Fsr2	r28
#define Fsr1	r27
#define Mergelo32	r25
#define Mergehi32	r24
#define Temp	r26

// special zero - page locations used within alltraps
	SV_PSR = 0xffffff80
	SV_FIR = 0xffffff84
	SV_R1 =  0xffffff88
	SV_ISP = 0xffffff8c
	SV_KSP = 0xffffff90
	SV_DB =  0xffffff94
	SV_TSP = 0xffffff98
	SV_TSP1 = 0xffffff9c

	RS_PSR = 0xffffffa0
	RS_FIR = 0xffffffa4
	RS_R1  = 0xffffffa8
	RS_ISP  = 0xffffffac
	SV_R31	= 0xffffffb0
	
	LOW_PG = 0x0fff
	DB_ATE = 1
	FSR_BITS = 1		// Flush zero, no traps, round nearest
	FSR_MU_AU = 0x2200	// FSR MU and AU bits, for Bug 37 workaround

//
//	Interrupt/context 0 stack and whatnot.
//
//	Keep this PAGE ALIGNED!
//
	.data
	_sdata:	.globl _sdata	// Marks start of all data.
	.blkb	4096	// Space for task 0 stack.
	df_stack:	.globl df_stack
	intstack:	.globl intstack	// top of task 0 stack

	_bad:	.globl _bad
	.blkb	8192	// Trashable 8K band for flush(), reset_initialization()

//
//	Bootstrap pages:
//
//	The first page(s) of code is designed to be run with a virtual to
//	physical address 1:1 mapping.  This code sets up the initial VM state.
//
//	Subsequent pages are mapped to virtual addresses 0xFxxxxxxx.
//
	.text
_stext:	.globl _stext		// Used when configuring MMU, start of text seg.
pstart:	.globl pstart		// label used by linker to set entry point

#define	AddrMask	r5
#define SlotAddr	r6
#define NDCSR_KEN860		0x00001000
#define NDCSR_BE_IEN		0x00000010

	ADDR_MASK = 0x0FFFFFFF	// mask out high nibble, for slot address

// Slot dependent addresses
	DRAM_BASE = 0x08000000	// Slot gets ORed into high nibble later

// Slot independent addresses
	ND_SLOTID = 0xFF800030	// Address of MC reg holding our Slot ID
	ND_CSR0	=   0xFF800000	// Address of MC CSR 0

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
	// CHIP BUG #48 Workaround not needed here.  VM isn't up yet.
	st.c	r16,dirbase	// 4 Kb pages, default cache, no ATE yet
	NOP6
	
	// Inhibit interrupts and the cache by clearing the MC CSR 0
	or	NDCSR_BE_IEN, r0, r17
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
	// There's nothing to flush, but this sets the cache tag bits
	// to a sane state.
	call	reset_initialization
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

	//
	// Set up physical address boot stack.
	//
	orh	h%intstack,r0,sp
	or	l%intstack,sp,sp	// Load virtual address
	and	AddrMask, sp, sp	// Convert to physical address
	or	SlotAddr, sp, sp
	adds	-16,sp,sp
	adds	8,sp,fp			// Build bogus stack frame
	st.l	fp,8(sp)
	st.l	r0,12(sp)		// Bogus return address, for debugger
	
	//
	// This will be a PC relative branch when assembled, so it should work...
	// _early_start returns the physical address of the PDE in r16.
	//
	call	_early_start	// Build and configure PTEs, in i860_init.c
	nop

//
//	Transfer to VM addressing.  r16 holds phys addr of kpde from _early_start.
//
	ld.c	dirbase,r17
	and	0xfff,r17,r17		// Save any neat bits set in dirbase
	or	DIR_ITI+DIR_ATE,r17,r17	// Invalidate TLB, enable address translation
	andnot	0xfff,r16,r16		// These bits SHOULD be zero already...
	or	r17,r16,r16		// Fold in saved neat bits and addr.
	.align 32		// Changing ITI and DTB, so meet all the constraints...
	// CHIP BUG #48 Workaround not needed here, as VM hasn't been running.
	st.c	r16,dirbase		// And switch to virtual addressing.
	NOP6

//
//	Jump to the virtual address that we should continue execution at...
//	All pages past this boot section reside at the virtual addresses the
//	code was linked for.
//
	orh	h%_vstart,r0,r20	// Load the virtual (linked) address
	or	l%_vstart,r20,r20	// we will continue at.
	nop	
	bri	r20			// Transfer from 1:1 mapped to virtual addr
	nop

	.text

//
// reset_initialization():
//
//	Post-RESET cache conditioning.
//	See Intel i860 PRM page 5-15, example 5-2 for 'details'
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
#define R_save_psr	r29
#define R_save_fsr	r30

//
//	reset_initialization()
//
reset_initialization:
//	Disable traps and interrupts during init.
	ld.c	psr, R_save_psr
	andnot	PSR_IM,R_save_psr,r16
	st.c	r16, psr
	nop
	ld.c	fsr, R_save_fsr
	or	FSR_BITS,r0,r16
	st.c	r16, fsr
//	Enable the cache, as required by CHIP BUG 43 Workaround
	orh	ha%ND_CSR0, r0, r16
	or	NDCSR_KEN860+NDCSR_BE_IEN, r0, r17
	st.l	r17, l%ND_CSR0(r16)		// I hope this works...
//	The init code, from the Intel recommended workaround.
	ld.c	dirbase, r17		// Load dirbase into r17
	adds	-1,r0, r19		// Loop decrement
	or	127, r0, r20		// Loop counter
	orh	h%_bad-32, r0,	r16	// r16 gets address minus 32 of flush area
	or	l%_bad-32, r16,	r16

	// First half of cache flush
	andnot	0xF00,r17,r18		// Clear RC, RB, allows entry w any RC,RB
	or	0x800,r18,r18		// Set RC == 2
	bla	r19,r20,flush1		// One time to init LCC
	st.c	r18,dirbase		// Store back dirbase with RC==2, RB==0

flush1:
	bla	r19,r20,flush1		// Loop on next insn 128 times
	    flush	32(r16)++	// Flush and autoincrement

	// Second half of cache flush, done by changing RB
	or	0x900,r18,r18		// RC==2, RB==1
	or	127, r0, r20		// Reload loop counter
	bla	r19,r20,flush2		// One time to init LCC
	st.c	r18,dirbase		// Store back dirbase with RC==2, RB==1

flush2:
	bla	r19,r20,flush2		// Loop on next insn 128 times
	    flush	32(r16)++	// Flush and autoincrement

// End of cribbed code.
//	Disable the cache
	or	NDCSR_BE_IEN, r0, r17
	orh	ha%ND_CSR0, r0, r16
	st.l	r17, l%ND_CSR0(r16)		// I hope this works...
//	Restore traps and interrupts
	st.c	R_save_fsr, fsr
	st.c	R_save_psr, psr
	bri	r1
	    st.c	r17, dirbase		// Restore saved dirbase
	
//
//	End of 1:1 mapped boot page(s).
//

//
// Virtual Memory startup.  The following code must be aligned on a page
// boundry.  It marks the start of code mapped to address 0xFxxxxxxx.
//
	.align 4096
_vstart:	.globl _vstart
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
	orh	h%1f,r0,r16	// and return address
	or	l%1f,r16,r16
	st.l	r16,12(sp)

//	Enable interrupts, cache
	call	_i860_start		// In i860_init.c
	nop

//	Transfer to kernel main
	mov	sp,r16		// Stack pointer
	mov	r0,r17
	mov	r0,r18
	call	_kern_main	// Transfer to kernel main.
1:	nop
	// Fall through.
_exit:	.globl	_exit
all_done:
	call	__exit
	nop
	br	all_done
	nop
//
//	The trap handler - All traps and interrupts vectors through here
//
	.globl	alltraps
alltraps:
// branch here from 0xffffff00

// save a few registers and the pc (without causing more traps)
// only place we have to store stuff without using a register is
// offset(r0) e.g. first or last 32k
	nop			// CHIP BUG
 	nop			// CHIP BUG
	st.l	r1,SV_R1(r0)
	ld.c	db,r1
	st.l	r1,SV_DB(r0)
	st.c	r0,db
	ld.c	fir,r1
	st.l	r1,SV_FIR(r0)
	ld.c	psr,r1		//r1 = PSR
	st.l	r1,SV_PSR(r0)
	st.c	r0,psr
	st.l	sp,SV_ISP(r0)
// switch stacks if necessary
	st.l	r31,SV_R31(r0)
	orh	0xF000,r0,r31
	subu	r31,sp,r0	// Is SP > 0xF0000000 => Is SP in the kernel already?
	bnc	waskern
	orh	ha%_P,r0,r31	// Fetch kernel stack pointer from P->p_addr, the
	ld.l	l%_P(r31),r1	// first element in the struct pointed to by P.
	ld.l	r1(r0),sp
waskern:
	nop			//fix chip timing bug
	andnot	0xf,sp,sp	//re-align sp to quad boundary
	ld.l	SV_R31(r0),r31	// Get R31 back
	ld.l	SV_TSP(r0),r1
	st.l	r1,SV_TSP1(r0)
	ld.l	SV_PSR(r0),r1
	st.l	sp,SV_TSP(r0)

// now that we have a valid stack, save more state
	adds	-400,sp,sp	// Allocate stack space for 400 bytes of state info!

	st.l	r16,(4*R16)(sp)
	st.l	r17,(4*R17)(sp)
	st.l	r18,(4*R18)(sp)
	st.l	r19,(4*R19)(sp)
	st.l	r20,(4*R20)(sp)
	st.l	r21,(4*R21)(sp)
	st.l	r22,(4*R22)(sp)
	st.l	r23,(4*R23)(sp)
	st.l	r24,(4*R24)(sp)
	st.l	r25,(4*R25)(sp)
	st.l	r26,(4*R26)(sp)
	st.l	r27,(4*R27)(sp)
	st.l	r28,(4*R28)(sp)
	st.l	r29,(4*R29)(sp)
	st.l	r30,(4*R30)(sp)
	st.l	r31,(4*R31)(sp)

	st.l	r4,(4*R4)(sp)
	st.l	r5,(4*R5)(sp)
	st.l	r6,(4*R6)(sp)
	st.l	r7,(4*R7)(sp)
	st.l	r8,(4*R8)(sp)
	st.l	r9,(4*R9)(sp)
	st.l	r10,(4*R10)(sp)
	st.l	r11,(4*R11)(sp)
	st.l	r12,(4*R12)(sp)
	st.l	r13,(4*R13)(sp)
	st.l	r14,(4*R14)(sp)
	st.l	r15,(4*R15)(sp)

	st.l	r0,(4*R0)(sp)
	ld.l	SV_R1(r0),r16
	st.l	r16,(4*R1)(sp)
	ld.l	SV_ISP(r0),r16
	st.l	r16,(4*SP)(sp)
	st.l	fp,(4*FP)(sp)


// save floating registers

	fst.q	f0,(4*(FREGS+0))(sp)
	fst.q	f4,(4*(FREGS+4))(sp)
	fst.q	f8,(4*(FREGS+8))(sp)
	fst.q	f12,(4*(FREGS+12))(sp)
	fst.q	f16,(4*(FREGS+16))(sp)
	fst.q	f20,(4*(FREGS+20))(sp)
	fst.q	f24,(4*(FREGS+24))(sp)
	fst.q	f28,(4*(FREGS+28))(sp)

#if ! defined(NO_P_SV)
//
// save pipeline states
//	We use the pipelines in kernel mode in our graphics engine,
//	so we can't shave any ticks this way.  Maybe if Leo and Ted
//	change their minds...
//
//	and	PSR_PU,r1,r0	// save if necessary
//	bc	end_save_pipes
save_pipes:
	orh		ha%DoubleOne,r0,r31
	fld.d		l%DoubleOne(r31),f4	// set doubleprecision 1.0
	st.l		r0,-4(r0)		// CHIP BUG #46, Workaround #3
	ld.c		fsr, Fsr3		// Save third stage result status
	andnot		0x20, Fsr3, Temp	// Clear FTE bit
	st.c		Temp, fsr		// Disable FP traps
	pfmul.ss	f0, f0, Mres3		// Save third stage M result
	pfadd.ss	f0, f0, Ares3		// Save third stage A result
	pfld.d		l%DoubleOne(r31), Lres	// save third stage pfld result
	fst.d		Lres, (PSV_L3*4)(sp)	// ... in memory
	ld.c		fsr, Fsr2		// Save second stage result status
	pfmul.ss	f0, f0, Mres2		// Save second stage M result
	pfadd.ss	f0, f0, Ares2		// Save second stage A result
	pfld.d 		l%DoubleOne(r31), Lres	// save third stage pfld result
	fst.d		Lres, (PSV_L2*4)(sp)	// ... in memory
	ld.c		fsr, Fsr1		// Save first stage result status
	pfmul.ss	f0, f0, Mres1		// Save first stage M result
	pfadd.ss	f0, f0, Ares1		// Save first stage A result
	pfld.d 		l%DoubleOne(r31), Lres	// save third stage pfld result
	fst.d		Lres, (PSV_L1*4)(sp)	// ... in memory
	pfiadd.dd	f0, f0, Ires1		// Save vector-integer result
// Save KR, KI, T, and MERGE
	andnot		0x2c, Fsr1, Temp	// Clear RM, clear FTE.
	or		4, Temp, Temp		// Set RM=01, to round down, so -0
	st.c		Temp, fsr		// preserved when added to f0
	r2apt.dd	f0, f4, f0		// M 1st stage contains KR, 1.0
						// A 1st stage contains T, 0.0 (I hope)
	i2p1.dd		f0, f4, f0		// M 1st stage contains KI, 1.0
	pfmul.dd	f0, f0, KR		// Save KR register
	pfmul.dd	f0, f0, KI		// Save KI register
	pfadd.dd	f0, f0, f0		// Adder 3rd stage gets T
	pfadd.dd	f0, f0, T		// Save T register
	form		f0, f2			// Save MERGE register
	fxfr		f2, Mergelo32
	fxfr		f3, Mergehi32
// Now dump the saved machine state to the stack
	fst.d		Mres3, (4*PSV_M3)(sp)
	fst.d		Mres2, (4*PSV_M2)(sp)
	fst.d		Mres1, (4*PSV_M1)(sp)
	fst.d		Ares3, (4*PSV_A3)(sp)
	fst.d		Ares2, (4*PSV_A2)(sp)
	fst.d		Ares1, (4*PSV_A1)(sp)
	fst.d		Ires1, (4*PSV_I1)(sp)
	fst.d		KR, (4*SPC_KR)(sp)
	fst.d		KI, (4*SPC_KI)(sp)
	fst.d		T, (4*SPC_T)(sp)
	st.l		Mergelo32, (4*SPC_MERGE)(sp)
	st.l		Mergehi32, (4*(SPC_MERGE+1))(sp)
	st.l		Fsr3, (4*PSV_FSR)(sp)
	st.l		Fsr2, (4*PSV_FSR2)(sp)
	st.l		Fsr1, (4*PSV_FSR1)(sp)
end_save_pipes:
#endif
	adds	16,sp,sp	// Pop stack up to base of saved regs.
				// This way we can use reg fields in insns
				// to index into saved regs.  We push the stack
				// back to normal before calling _trap()

// figure out trap reason (put in r20)
	ld.l	SV_FIR(r0),r16
	and	PSR_IN,r1,r0
	bc	no_intr
	or	T_INTRPT,r0,r20
	br	got_ttype
	nop
no_intr:
	and	PSR_IAT,r1,r0
	bc	no_iat
	or	T_INSFLT,r0,r20
	br	got_ttype
	mov	r0,r19		// Couldn't be a write.  Write to insn space forbidden.
no_iat:
	and	PSR_IT,r1,r0
	bc	no_it
// was an IT, but what kind?
	and	PSR_DIM,r1,r31	// In DIM? Check other half of pair
	bte	r31,r0,1f

	addu	4,r16,r31	// Select integer half	
	xor	4,r31,r31	// Flip addr to big-endian for fetch
	br	2f
	    ld.l	0(r31),r17	// this can't fault, already checked for IAT
1:
	xor	4,r16,r31	// Flip addr to big-endian for fetch
	ld.l	0(r31),r17	// this can't fault, already checked for IAT
2:
//	xor	4,r16,r16	// Flip addr to big-endian for fetch
//	ld.l	0(r16),r17	// this can't fault, already checked for IAT
//	xor	4,r16,r16	// Flip addr back to little-endian

	xorh	0x4400,r17,r0	// trap r0,r0,r0
	bnc	not_bpt
	or	T_BPTFLT,r0,r20
	br	got_ttype
	nop
not_bpt:
	andh	0xffff,r17,r31	// insn in r17
	xorh	0x47e0,r31,r0	// trap r31,r31,r0
	bnc	not_sysc
	or	T_SYSCALL,r0,r20
	br	got_ttype
	nop
not_sysc:
	or	T_PRIVINFLT,r0,r20
	br	got_ttype
	nop
no_it:
	and	PSR_FT,r1,r0
	bc	no_ft
	or	T_ARITHTRAP,r0,r20
	br	got_ttype
	nop
no_ft:
	and	PSR_DAT,r1,r0
	bc	no_tbits	//should never happen, but CHIP BUG
	or	T_PAGEFLT,r0,r20
// figure out virtual address (r18)
//		r/w		(r19)
//		align required	(r21)
	and	PSR_DIM,r1,r31	// In DIM? Check other half of pair
	bte	r31,r0,1f

	addu	4,r16,r31	// Select integer half	
	xor	4,r31,r31	// Flip addr to big-endian for fetch
	br	2f
	    ld.l	0(r31),r17	// this can't fault, already checked for IAT
1:
	xor	4,r16,r31	// Flip addr to big-endian for fetch
	ld.l	0(r31),r17	// this can't fault, already checked for IAT
2:
	andh	0x2000,r17,r31	// Is this a load or store?
	btne	r31,r0,not_ld_st
	// Found ld/st. Opcode in r17
//double check must be 000x.0x or 000x.11 (dont trust fir)
	andh	0xe800,r17,r21
	bc	ls_ok
	andh	0xec00,r17,r21
	xorh	0x0c00,r21,r0
	bc	ls_ok
	or	T_COMPATFLT,r0,r20
	br	got_ttype
	nop
ls_ok:
// ld or st
	mov	r0,r21
	andh	0x1000,r17,r0
	bc	got_ls_aln
	or	1,r21,r21
	and	1,r17,r0
	bc	got_ls_aln
	or	2,r21,r21
got_ls_aln:
	andh	0x0800,r17,r0
	bc	is_ld
// st
	or	1,r0,r19
// st EA
// const+src2 where const = dest|low11 sign extended
	shr	19,r17,r22
	and	0x7c,r22,r22
	ld.l	r22(sp),r22	//src2

	shr	5,r17,r23
	and	0xf800,r23,r23
	and	0x07ff,r17,r24
	or	r23,r24,r23
	shl	16,r23,r23
	shra	16,r23,r23
	and	1,r21,r24
	nop
	andnot	r24,r23,r23	// adjusted const

	br	got_datp
	adds	r23,r22,r18	// EA

// ld
is_ld:
	mov	r0,r19
// ld EA
	and	1,r21,r25
	mov	r0,r26

// at this point, we have:
//	r17 - the instruction
//	r25 - low order bits to throw away if const(reg)
//	r26 - bit 0 set if we need auto-increment fixup
//	r19,r21 already set
gen_ea:
	shr	19,r17,r27
	and	0x7c,r27,r27	//index of src2 * 4
	ld.l	r27(sp),r22	//src2

	andh	0x0400,r17,r0
	bc	gen_r_r
// const src1
	shl	16,r17,r23
	shra	16,r23,r23

	nop
	andnot	r25,r23,r23	// toss some low bits

	br	ai_fix
	nop

// reg src1
gen_r_r:
	shr	9,r17,r23
	and	0x7c,r23,r23
	ld.l	r23(sp),r23	// src1

// value of src2 is in r22
// value of src1 is in r23
// index of src2 * 4 is in r27
// flag for autoinc fix in r26
ai_fix:
	and	1,r26,r0
	bc	no_ai_fix
	subs	r22,r23,r22
	adds	r27,sp,r26
	st.l	r22,0(r26)
	
no_ai_fix:
	br	got_datp
	adds	r23,r22,r18

not_ld_st:
// must be one of:
//	fld,fst	0010.xx
//	pst	0011.11
	andh	0xf000,r17,r21
	xorh	0x2000,r21,r0
	bc	fdat_ok
	andh	0xfc00,r17,r21
	xorh	0x3f00,r21,r0
	bc	fdat_ok
	or	T_COMPATFLT,r0,r20
	br	got_ttype
	nop
fdat_ok:
	and	1,r17,r26	//need auto-increment fix?
	or	3,r0,r21
	and	2,r17,r0
	bnc	got_f_aln
	or	4,r21,r21
	and	4,r17,r0
	bc	got_f_aln
	or	8,r21,r21
got_f_aln:
	and	7,r21,r25
	andh	0x1000,r17,r0
	bnc	is_pix
	andh	0x0800,r17,r0
	bc	is_fld
// fst
	or	1,r0,r19
	br	gen_ea
	nop
// fld or pfld
is_fld:
	mov	r0,r19
	br	gen_ea
	nop
// pst
is_pix:
	or	1,r0,r19
	br	gen_ea
	nop

got_datp:
	br	got_ttype
	nop
no_tbits:
	or	T_ARITHTRAP,r0,r20

got_ttype:
	adds	-16,sp,sp	// Put stack back to standard offset 
	st.l	r1,0(sp)	//PSR
	ld.l	SV_FIR(r0),r16
	st.l	r16,4(sp)	//FIR
	st.l	r20,8(sp)	//trap type
	ld.l	SV_DB(r0),r16
	st.l	r16,12(sp)	//DB
	
// Clear the trap flags in the PSR, so we can catch any new problems in the 
// trap handler itself.
	andnot	0x1f00,r1,r30
	st.c	r30,psr

// call trap()

	mov	sp,r16
	call	_trap
	nop
//	call	_flush_inv	// We should be flushing in all the right places now
//	nop

// restore state
	.globl	ret_user
ret_user:
	ld.c	psr,r1		//turn off interrupts for now
	nop
	andnot	PSR_IM,r1,r1
	st.c	r1,psr
	nop
	ld.l	0(sp),r1	//resume PSR
	andnot	PSR_IM,r1,r1	//the bri will set PSR_IM

	st.l	r1,RS_PSR(r0)	// store the resume PSR

	ld.l	4(sp),r16
	st.l	r16,RS_FIR(r0)	//resume PC
	ld.l	12(sp),r16
	st.c	r16,db		//resume DB

#if !defined(NO_P_SV)
//
// restore pipe state
//	We use pipes in kernel mode in our graphics engine,
//	so this optimization can't be done right now.
//
//	and	PSR_PU,r1,r0	// save if necessary
//	bc	end_restore_pipes

restore_pipes::
	fld.d		(4*PSV_M3)(sp), Mres3
	fld.d		(4*PSV_M2)(sp), Mres2
	fld.d		(4*PSV_M1)(sp), Mres1
	fld.d		(4*PSV_A3)(sp), Ares3
	fld.d		(4*PSV_A2)(sp), Ares2
	fld.d		(4*PSV_A1)(sp), Ares1
	fld.d		(4*PSV_I1)(sp), Ires1
	fld.d		(4*SPC_KR)(sp), KR
	fld.d		(4*SPC_KI)(sp), KI
	fld.d		(4*SPC_T)(sp), T
	ld.l		(4*SPC_MERGE)(sp), Mergelo32
	ld.l		(4*(SPC_MERGE+1))(sp), Mergehi32
	ld.l		(4*PSV_FSR)(sp), Fsr3
	ld.l		(4*PSV_FSR2)(sp), Fsr2
	ld.l		(4*PSV_FSR1)(sp), Fsr1

	st.c		r0, fsr		// Clear FTE
	st.l		r0,-4(r0)		// CHIP BUG #46, Workaround #3
// Restore MERGE
	shl		16, Mergelo32, Temp	// Move low 16 bits to high 16
	ixfr		Temp, f2
	shl		16, Mergehi32, Temp	// Move low 16 bits to high 16
	ixfr		Temp, f3
	ixfr		Mergelo32, f4
	ixfr		Mergehi32, f5
	faddz		f0, f2, f0	// Merge low 16s
	faddz		f0, f4, f0	// Merge high 16s
// Restore KR, KI, and T
	orh		ha%SingleOne,r0,r31
	fld.l		l%SingleOne(r31),f2	// get single precision 1.0
	orh		ha%DoubleOne,r0,r31
	fld.d		l%DoubleOne(r31),f4	// get double precision 1.0
	pfmul.dd	f4, T, f0		// Put value of T in M 1st stage
	r2pt.dd		KR, f0, f0		// Load KR. advance T
	i2apt.dd	KI, f0, f0		// Load KI and T
// Restore third stage
	andh		0x2000, Fsr3, r0	// test adder result precision ARP
	bc.t		L0			// taken if it was single
	pfamov.ss	Ares3, f0		// Insert single precision result
	pfamov.dd	Ares3, f0		// Insert double precision result
L0:
	andh		0x400, Fsr3, r0		// Test load result precision LRP
	bc.t		L1			// taken if it was single
	pfld.l		(4*(PSV_L3+1))(sp), f0	// insert single precision result
	pfld.d		(4*PSV_L3)(sp), f0	// insert double precision result
L1:
	andh		0x1000, Fsr3, r0	// Test multiplier result precision MRP
	bc.t		L2			// taken if it was single
	pfmul.ss	Mres3, f2, f0		// Insert single precision result
	pfmul3.dd	Mres3, f4, f0		// Insert double precision result
L2:
	or		0x10, Fsr3, Temp	// Set U (update) bit so st.c will
						// update status bits in the pipeline
	andnot		0x20, Temp, Temp	// Clear FTE bit to avoid traps
	andnot		FSR_MU_AU, Temp, Temp	// CHIP BUG 37: Assume FZ always set
	st.c		Temp, fsr		// Update stage 3 result status
// Restore second stage
	andh		0x2000, Fsr2, r0	// test adder result precision ARP
	bc.t		L3			// taken if it was single
	pfamov.ss	Ares2, f0		// Insert single precision result
	pfamov.dd	Ares2, f0		// Insert double precision result
L3:
	andh		0x400, Fsr2, r0		// Test load result precision LRP
	bc.t		L4			// taken if it was single
	pfld.l		(4*(PSV_L2+1))(sp), f0	// insert single precision result
	pfld.d		(4*PSV_L2)(sp), f0	// insert double precision result
L4:
	or		0x10, Fsr2, Temp	// Set U (update) bit so st.c will
						// update status bits in the pipeline
	andnot		0x20, Temp, Temp	// Clear FTE bit to avoid traps
	andnot		FSR_MU_AU, Temp, Temp	// CHIP BUG 37: Assume FZ always set
	andh		0x1000, Fsr2, r0	// Test multiplier result precision MRP
	bc.t		L5			// taken if it was single
	pfmul.ss	Mres2, f2, f0		// Insert single precision result
	pfmul3.dd	Mres2, f4, f0		// Insert double precision result
L5:
	st.c		Temp, fsr		// Update stage 2 result status
// Restore first stage
	andh		0x1000, Fsr1, r0	// Test multiplier result precision MRP
	bc.t		L6			// taken if it was single
	pfmul.ss	Mres1, f2, f0		// Insert single precision result
	pfmul3.dd	Mres1, f4, f0		// Insert double precision result
L6:	
	andh		0x2000, Fsr1, r0	// test adder result precision ARP
	bc.t		L7			// taken if it was single
	pfamov.ss	Ares1, f0		// Insert single precision result
	pfamov.dd	Ares1, f0		// Insert double precision result
L7:
	andh		0x400, Fsr1, r0		// Test load result precision LRP
	bc.t		L8			// taken if it was single
	pfld.l		(4*(PSV_L1+1))(sp), f0	// insert single precision result
	pfld.d		(4*PSV_L1)(sp), f0	// insert double precision result
L8:
	andh		0x800, Fsr1, r0		// test vector-integer result precision
	bc.t		L9			// taken if single precision
	pfiadd.ss	f0, Ires1, f0		// Insert single precision result
	pfiadd.dd	f0, Ires1, f0		// Insert double precision result
L9:
	or		0x10, Fsr1, Fsr1	// Set U (update) bit so st.c will
						// update status bits in the pipeline
	andnot		FSR_MU_AU, Fsr1, Fsr1	// CHIP BUG 37: Assume FZ always set
	st.c		Fsr1, fsr		// Update stage 1 result status
	st.c		Fsr3, fsr		// Restore non-pipelined FSR status

end_restore_pipes:
#endif

// restore fregs
	fld.q	(4*(FREGS+0))(sp),f0
	fld.q	(4*(FREGS+4))(sp),f4
	fld.q	(4*(FREGS+8))(sp),f8
	fld.q	(4*(FREGS+12))(sp),f12
	fld.q	(4*(FREGS+16))(sp),f16
	fld.q	(4*(FREGS+20))(sp),f20
	fld.q	(4*(FREGS+24))(sp),f24
	fld.q	(4*(FREGS+28))(sp),f28

	adds	16,sp,sp

	ld.l	4(sp),r16
	st.l	r16,RS_R1(r0)	//resume R1
	ld.l	8(sp),r16
	st.l	r16,RS_ISP(r0)	//resume SP
	ld.l	12(sp),fp	//resume FP
	adds	16,sp,sp

	ld.l	0(sp),r4
	ld.l	4(sp),r5
	ld.l	8(sp),r6
	ld.l	12(sp),r7
	ld.l	16(sp),r8
	ld.l	20(sp),r9
	ld.l	24(sp),r10
	ld.l	28(sp),r11
	ld.l	32(sp),r12
	ld.l	36(sp),r13
	ld.l	40(sp),r14
	ld.l	44(sp),r15
	adds	48,sp,sp

	ld.l	0(sp),r16
	ld.l	4(sp),r17
	ld.l	8(sp),r18
	ld.l	12(sp),r19
	ld.l	16(sp),r20
	ld.l	20(sp),r21
	ld.l	24(sp),r22
	ld.l	28(sp),r23
	ld.l	32(sp),r24
	ld.l	36(sp),r25
	ld.l	40(sp),r26
	ld.l	44(sp),r27
	ld.l	48(sp),r28
	ld.l	52(sp),r29
	ld.l	56(sp),r30
	ld.l	60(sp),r31
	adds	64,sp,sp

	adds	256,sp,sp

	or	PSR_IT,r1,r1
	st.c	r1,psr		//set return PSR (with trap bit set)
	st.l	sp,SV_KSP(r0)	//save kernel sp
	ld.l	RS_ISP(r0),sp	// return SP
	ld.l	RS_FIR(r0),r1	// return PC
	ld.l	RS_R1(r0),r0	// CHIP BUG Step B2, Bug 10
	bri	r1
	ld.l	RS_R1(r0),r1
//
//	End of trap handler code
//


	.globl	_system_bootstrap
_system_bootstrap:
	call	_kern_main
	nop
//
//	Should never get here
//
	orh	h%panic_msg,r0,r16
	or	l%panic_msg,r16,r16
setup_hlt:
	call	_panic
	nop
	br	setup_hlt
	nop

	.globl	_thread_bootstrap
_thread_bootstrap:
//	call	_spl0			// going to user mode
	br	ret_user
	nop

	.data
panic_msg:
	.string	"First thread returned\n"
	.byte	0
panic_msg1:
	.string	"Jump to tss failed\n"
	.byte	0

wierd_death1:
	.string	"0x%X\n"
	.byte 0
//
// Used by trap handler for pipeline save/restore
//
	.align 4 
SingleZero:	.float  0.0
SingleOne:	.float  1.0
	.align 8
DoubleOne:	.double 1.0
	.text

// ********************************** END OF MACH vstart *********

//
//	routines to get various special registers
	.globl	_current_pmap
_current_pmap:
	ld.c	dirbase,r16
	andnot	0xfff,r16,r16		// Strip out control bits
	bri	r1	
	orh	0xf000,r16,r16		// Replace the slot nibble with a virt. addr

	.globl	_get_dirbase
_get_dirbase:
	ld.c	dirbase,r16
	bri	r1
	nop
	.globl	_get_fp
_get_fp:
	mov	fp,r16
	bri	r1
	nop
	.globl	_get_epsr
_get_epsr:
	.align 32		// CHIP BUG Step B2, Bug 25
	ixfr	r0,f0
	ld.c	epsr,r16
	bri	r1
	nop
//
//	change to new dirbase
//
//	cant do any memory ld's between flush and changing dirbase!!
	.globl	_new_dirbase
_new_dirbase:
	mov	r1,r28		//not used by _flush: return address
	mov	r16,r29		//not used by _flush: new dirbase
	call	_flush
	nop
	ld.c	dirbase,r18
	and	0xfff,r18,r18
	or	DIR_ITI,r18,r18
	nop
	andnot	0xfff,r29,r29
	or	r29,r18,r18
	.align 32		// Changing DTB, so meet all the constraints...
	ixfr	r0, f0	// CHIP BUG #48 Workaround.  FP traps are always disabled.
	st.c	r18,dirbase
	NOP6
	bri	r28
	nop
//
// Invalidate the TLB:
//
//	On B step and later chips, we don't have to flush the cache before invalidating
//	the TLB for certain specific cases, including:
//
//		Setting a PTE dirty bit
//		Modifying the Present, User, Writable, or Accessed bit
//
//	IFF the page frame address was not changed and the PTE was not in the data
//	cache.  We guarantee in this implementation that PTEs are not cacheable.
//
	.globl	_invalidate_TLB
_invalidate_TLB:
	ld.c	dirbase,r16
	nop
	or	DIR_ITI,r16,r16
	.align 32		// Changing DTB, so meet all the constraints...
	ixfr	r0, f0	// CHIP BUG #48 Workaround.  FP traps are always disabled.
	st.c	r16,dirbase
	NOP6
	bri	r1
	nop

//
//	routine to turn on virtual memory
// (almost same as above -- may merge later)
	.globl	_go_virt
_go_virt:
	ld.c	dirbase,r17
	and	0xfff,r17,r17
	nop
	andnot	0xfff,r16,r16
	or	DIR_ITI+DIR_ATE,r16,r16
	or	r17,r16,r16
	.align 32		// Changing ITI and DTB, so meet all the constraints...
	// CHIP BUG #48 Workaround not neede, as VM hasn't been running yet.
	st.c	r16,dirbase
	NOP6
	bri	r1
	nop
//
//	bzero(adr, bcnt)
//
	.globl	_bzero
_bzero:
	or	r17,r0,r0
	bc	bzdone

	adds	-1,r0,r18	//r18 = -1 for bla
	or	r16,r17,r19
	and	3,r19,r0
	bc	bzdolongs	//long aligned addr and 4*x bcnt?

	adds	-1,r17,r17
	bla	r18,r17,blp
	nop
blp:
	st.b	r0,0(r16)
	bla	r18,r17,blp
	adds	1,r16,r16

	bri	r1
	nop
bzdolongs:
	shr	2,r17,r17
	adds	-1,r17,r17
	adds	-4,r16,r16
	bla	r18,r17,llp
	nop
llp:
	bla	r18,r17,llp
	fst.l	f0,4(r16)++

bzdone:
	bri	r1
	nop
//
//	memmove(to, from, nbytes)
//	
//	Swap the from and to registers and fall through to _bcopy
//
	.globl	_memmove
_memmove:
	or	r16,r0,r31	// Save the 'to' addr
	or	r17,r0,r16	// Move 'from' addr to 1st arg
	or	r31,r0,r17	// Move saved 'to' addr to 2nd arg and fall through
//
//	bcopy(from,to,nbytes)
	.globl	_bcopy
_bcopy:
	or	r0,r18,r0
	bc	bcdone

	adds	-1,r0,r28	//r28 = -1 for bla

	or	r16,r17,r19
	or	r18,r19,r19
	and	3,r19,r0
	bc	bcdolongs	//long aligned addr and 4*x bcnt?

	adds	-1,r18,r18
	bla	r28,r18,bcblp
	nop
bcblp:
	ld.b	0(r16),r29
	st.b	r29,0(r17)
	adds	1,r17,r17
	bla	r28,r18,bcblp
	adds	1,r16,r16

	bri	r1
	nop

bcdolongs:
	shr	2,r18,r18
	adds	-1,r18,r18
	bla	r28,r18,bcllp
	nop
bcllp:
	ld.l	0(r16),r29
	st.l	r29,0(r17)
	adds	4,r17,r17
	bla	r28,r18,bcllp
	adds	4,r16,r16

bcdone:
	bri	r1
	nop
//
//	thread_t current_thread()
//	{
//		return(active_thread[cpu_number()]);
//	}
//
//	.globl	_current_thread
//_current_thread:
//	orh	ha%_active_threads,r0,r17
//	bri	r1
//	ld.l	l%_active_threads(r17),r16
//
	.globl	_ALLOW_FAULT_START
_ALLOW_FAULT_START:

//
//	copyin(from,to,nbytes)
	.globl	_copyin
_copyin:
	mov	r1,r30
	call	userstring	//check that block is in user space
	mov	r16,r19
	bc.t	cpcdone
	adds	-1,r0,r16

copycom:
	or	r0,r18,r0
	bc	cpcdone

	adds	-1,r0,r28	//r28 = -1 for bla

	or	r16,r17,r19
	or	r18,r19,r19
	and	3,r19,r0
	bc	cpclongs	//long aligned addr and 4*x bcnt?

	adds	-1,r18,r18
	bla	r28,r18,cpcblp
	nop
cpcblp:
	ld.b	0(r16),r29
	st.b	r29,0(r17)
	adds	1,r17,r17
	bla	r28,r18,cpcblp
	adds	1,r16,r16

	bri	r30
	mov	r0,r16

cpclongs:
	shr	2,r18,r18
	adds	-1,r18,r18
	bla	r28,r18,cpcllp
	nop
cpcllp:
	ld.l	0(r16),r29
	st.l	r29,0(r17)
	adds	4,r17,r17
	bla	r28,r18,cpcllp
	adds	4,r16,r16

cpcdone:
	bri	r30
	mov	r0,r16

//
//	If a paging fault happens in copycom, control is returned here.
//
	.globl	_FAULT_ERROR
_FAULT_ERROR:
	adds	-1,r0,r16
	bri	r30
	nop


//
//	copyout(from,to,nbytes)
	.globl	_copyout
_copyout:
	mov	r1,r30
	call	userstring		//verify that block is in user space
	mov	r17,r19
	bc.t	cpcdone
	adds	-1,r0,r16
	br	copycom
	nop

//
//	isuser - check that address is in user space

isuser:				// MINUVADR is zero so don't check lower bound
//	orh	h%-VM_MIN_KERNEL_ADDRESS+1,r0,r31
//	or	l%-VM_MIN_KERNEL_ADDRESS+1,r31,r31
//	addu	r31,r19,r0

  subs r0,r0,r0

	bri	r1
	nop

//
//	userstring - same as isuser but for a string

userstring:
//	orh	h%-VM_MIN_KERNEL_ADDRESS+1,r0,r31
//	or	l%-VM_MIN_KERNEL_ADDRESS+1,r31,r31
//	addu	r31,r19,r19
//	bnc.t	usr1
//	addu	r18,r19,r0
//usr1:

  subs r0,r0,r0

	bri	r1
	nop

	.globl	_ALLOW_FAULT_END
_ALLOW_FAULT_END:

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
	orh	ha%ND_SLOTID, r0, r31
	ld.l	l%ND_SLOTID(r31), r18
	
	orh	h%ADDR_MASK, r0, r17
	or	l%ADDR_MASK, r17, r17

	and	r16, r17, r16	// Mask out high nibble
	shl	28, r18, r18	// Move slot # to high nibble
	or	r16, r18, r16	// OR in the slot number for the high nibble
1:
	bri	r1
	nop	

//
//	interrupt handling
//
	.globl	_splhigh
_splhigh:
	.globl	_splclock
_splclock:
	.globl	_spl7
_spl7:
	.globl	_splsched
_splsched:
	.globl	_splvm
_splvm:
	.globl	_spl6
_spl6:
	.globl	_splbio
_splbio:
	.globl	_splimp
_splimp:
	.globl	_splnet
_splnet:
	.globl	_spl5
_spl5:
	.globl	_spltty
_spltty:
	br	splcom
	mov	r0,r17			// Mask interrupts

	.globl	_splsoftclock
_splsoftclock:
	.globl	_spl1
_spl1:
	.globl	_spl0
_spl0:

//	orh	ha%_ki_enable,r0,r31
//	ld.l	l%_ki_enable(r31),r17
//	shl	4,r17,r17

	or	PSR_IM,r0,r17		// Permit interrupts

splcom:
	ld.c	psr,r18
	and	PSR_IM,r18,r16		// Return old interrupt mode
	nop
	andnot	PSR_IM,r18,r18
	or	r17,r18,r18		// Load in new interrupt mode
	bri	r1
	st.c	r18,psr

	.globl	_splx
_splx:
	ld.c	psr,r17
	and	PSR_IM,r16,r16		// Isolate mode to be restored
	or	r16,r17,r17		// Restore old mode
	bri	r1
	st.c	r17,psr
//
//	These two routines are only used by the debugger, which is why
//	it is written separately from all the other spl's
//
	.globl	_splall
_splall:				// Block all interrupts
	mov	r0,r17
	ld.c	psr,r18
	and	PSR_IM,r18,r16
	nop
	andnot	PSR_IM,r18,r18
	or	r17,r18,r18
	bri	r1
	st.c	r18,psr

	.globl	_backall
_backall:
	ld.c	psr,r17
	and	PSR_IM,r16,r16
	or	r16,r17,r17
	bri	r1
	st.c	r17,psr
//
// abort():
//
//	Abort execution.  Really just a breakpoint trap.
//
_abort:	.globl	_abort
	trap	r0, r0, r0
	nop
	bri	r1
	nop
//
// alloca()
//
//	The ever popular sleazoid stack allocator. Callers in the 
//	kernel are on their own with regard to what happens when the
//	kernel stack is exhausted.
//
_alloca:	.globl	_alloca
	adds	15, r16, r16	// Round request to 16 byte bound
	andnot	15, r16, r16	// to keep our stack alignment correct.
	subs	sp, r16, sp	// Subtract new space from the stack.
	bri	r1
	    mov	sp, r16		// Return the newly allocated stack space

//
// DataTrap:
//
//	Debugger glue routine which sets a data access trap on a given
//	location.  Usage is DataTrap( addr, access ), where access is 1 for read,
//	2 for write, and 3 for R/W.
//
_DataTrap:	.globl _DataTrap
	and	0x3,r17,r17	// Filter down the access bits
	ld.c	psr,r18		// fetch current PSR
	andnot	0x3,r18,r18	// Zap current access bits
	or	r17,r18,r18	// Set new access bits
	st.c	r18, psr
	st.c	r16, db		// Set data address to break on.
	bri r1
	nop
//
//	Glue to connect the new context spawned by SpawnProc() with the target
//	coroutine function.
//
_CoInit:	.globl _CoInit
        adds	-16,sp,sp
        st.l	fp,8(sp)
        st.l	r1,12(sp)
        adds	8,sp,fp

	mov	r15,r16		// load up arg to coroutine function.
	calli	r14		// and transfer to the coroutine.
	nop
				// The coroutine returned!  Destroy the process.
	orh	ha%_P,r0,r31
	ld.l	l%_P(r31),r16	// Get the current process pointer
	call	_DestroyProc	// Terminate the process.
	nop			// Never returns.
//










