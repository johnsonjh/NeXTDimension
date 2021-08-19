/*
***********************************************************************
*                                                                     *
*             INTEL CORPORATION PROPRIETARY  INFORMATION              *
*                                                                     *
*     This software is supplied under  the  terms  of  a  licence     *
*     agreement    or   non-disclosure   agreement   with   Intel     *
*     Corporation and  may not be copied  nor disclosed except in     *
*     accordance with the terms of that agreement.                    *
*                                                                     *
*                       Copyright 1989 Intel                          *
*                                                                     *
***********************************************************************
*/

#include <setjmp.h>

	.text
#if 0
	.globl	_save_context
_save_context:
	mov	r1,r20		//save our ret addr
	call	_get_pcb_context
	nop
	mov	r16,r19
	call	_splhigh		//get old priority level
	nop
	st.l	r16,60(r19)
	call	_splx
	nop
	// address of save area is in r19
	st.l	r4,0(r19)
	st.l	r5,4(r19)
	st.l	r6,8(r19)
	st.l	r7,12(r19)
	st.l	r8,16(r19)
	st.l	r9,20(r19)
	st.l	r10,24(r19)
	st.l	r11,28(r19)
	st.l	r12,32(r19)
	st.l	r13,36(r19)
	st.l	r14,40(r19)
	st.l	r15,44(r19)
	st.l	fp,48(r19)
	st.l	sp,52(r19)
	st.l	r20,56(r19)
	mov	r0,r16		//retval <- 0
	bri	r20
	nop
#endif
	.globl	_setjmp
_setjmp:
	mov	r1,r20		// Safe from spl code - return address
	mov	r16,r19		// Safe from spl code - save area pointer
	call	_splhigh	// Get old spl level
	nop
	st.l	r16,(4*JB_IPL)(r19)	// Save the SPL level
	call	_splx		// Back to original spl level
	nop
	// address of save area is in r19
	st.l	r4,(4*JB_R4)(r19)
	st.l	r5,(4*JB_R5)(r19)
	st.l	r6,(4*JB_R6)(r19)
	st.l	r7,(4*JB_R7)(r19)
	st.l	r8,(4*JB_R8)(r19)
	st.l	r9,(4*JB_R9)(r19)
	st.l	r10,(4*JB_R10)(r19)
	st.l	r11,(4*JB_R11)(r19)
	st.l	r12,(4*JB_R12)(r19)
	st.l	r13,(4*JB_R13)(r19)
	st.l	r14,(4*JB_R14)(r19)
	st.l	r15,(4*JB_R15)(r19)
	st.l	fp,(4*JB_FP)(r19)
	st.l	sp,(4*JB_SP)(r19)
	st.l	r20,(4*JB_R1)(r19)
	fst.l	f2,(4*JB_F2)(r19)
	fst.l	f3,(4*JB_F3)(r19)
	fst.l	f4,(4*JB_F4)(r19)
	fst.l	f5,(4*JB_F5)(r19)
	fst.l	f6,(4*JB_F6)(r19)
	fst.l	f7,(4*JB_F7)(r19)
	mov	r0,r16		//retval <- 0
	bri	r20
	nop

	.globl	_longjmp
_longjmp:
	mov	r16,r19		// Safe from spl code - save area pointer
	mov	r17,r20		// Safe from spl code - 'setjmp' return value
	call	_splhigh	// critical section
	nop
	// address of save area is in r19
	// return value is in r17
	ld.l	(4*JB_R4)(r19),r4
	ld.l	(4*JB_R5)(r19),r5
	ld.l	(4*JB_R6)(r19),r6
	ld.l	(4*JB_R7)(r19),r7
	ld.l	(4*JB_R8)(r19),r8
	ld.l	(4*JB_R9)(r19),r9
	ld.l	(4*JB_R10)(r19),r10
	ld.l	(4*JB_R11)(r19),r11
	ld.l	(4*JB_R12)(r19),r12
	ld.l	(4*JB_R13)(r19),r13
	ld.l	(4*JB_R14)(r19),r14
	ld.l	(4*JB_R15)(r19),r15
	ld.l	(4*JB_FP)(r19),fp
	ld.l	(4*JB_SP)(r19),sp
	fld.l	(4*JB_F2)(r19),f2
	fld.l	(4*JB_F3)(r19),f3
	fld.l	(4*JB_F4)(r19),f4
	fld.l	(4*JB_F5)(r19),f5
	fld.l	(4*JB_F6)(r19),f6
	fld.l	(4*JB_F7)(r19),f7
	ld.l	(4*JB_IPL)(r19),r16	// saved ipl
	call	_splx		// end of critical section
	nop
	ld.l	(4*JB_R1)(r19),r1	// return address
	mov	r20,r16
	bri	r1
	nop

// Kernel debugger versions
//
//	These don't save or restore the processor interrupt mode bit
//
	.globl	_kdbsetjmp
_kdbsetjmp:
	// address of save area is in r16
	st.l	r4,(4*JB_R4)(r16)
	st.l	r5,(4*JB_R5)(r16)
	st.l	r6,(4*JB_R6)(r16)
	st.l	r7,(4*JB_R7)(r16)
	st.l	r8,(4*JB_R8)(r16)
	st.l	r9,(4*JB_R9)(r16)
	st.l	r10,(4*JB_R10)(r16)
	st.l	r11,(4*JB_R11)(r16)
	st.l	r12,(4*JB_R12)(r16)
	st.l	r13,(4*JB_R13)(r16)
	st.l	r14,(4*JB_R14)(r16)
	st.l	r15,(4*JB_R15)(r16)
	st.l	fp,(4*JB_FP)(r16)
	st.l	sp,(4*JB_SP)(r16)
	st.l	r1,(4*JB_R1)(r16)
	fst.l	f2,(4*JB_F2)(r16)
	fst.l	f3,(4*JB_F3)(r16)
	fst.l	f4,(4*JB_F4)(r16)
	fst.l	f5,(4*JB_F5)(r16)
	fst.l	f6,(4*JB_F6)(r16)
	fst.l	f7,(4*JB_F7)(r16)
	mov	r0,r16		//retval <- 0
	bri	r1
	nop

	.globl	_kdblongjmp
_kdblongjmp:
	// address of save area is in r16
	// return value is in r17
	ld.l	(4*JB_R4)(r16),r4
	ld.l	(4*JB_R5)(r16),r5
	ld.l	(4*JB_R6)(r16),r6
	ld.l	(4*JB_R7)(r16),r7
	ld.l	(4*JB_R8)(r16),r8
	ld.l	(4*JB_R9)(r16),r9
	ld.l	(4*JB_R10)(r16),r10
	ld.l	(4*JB_R11)(r16),r11
	ld.l	(4*JB_R12)(r16),r12
	ld.l	(4*JB_R13)(r16),r13
	ld.l	(4*JB_R14)(r16),r14
	ld.l	(4*JB_R15)(r16),r15
	ld.l	(4*JB_FP)(r16),fp
	ld.l	(4*JB_SP)(r16),sp
	ld.l	(4*JB_R1)(r16),r1
	fld.l	(4*JB_F2)(r16),f2
	fld.l	(4*JB_F3)(r16),f3
	fld.l	(4*JB_F4)(r16),f4
	fld.l	(4*JB_F5)(r16),f5
	fld.l	(4*JB_F6)(r16),f6
	fld.l	(4*JB_F7)(r16),f7
	mov	r17,r16
	bri	r1
	nop
