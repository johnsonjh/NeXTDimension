	.text
	nop	// FIR - 4 pager check value
gcc_compiled.:
.text
	.align 4
.globl _ConstantRunMark32
_ConstantRunMark32:
	adds -16,sp,sp
	st.l fp,8(sp)
	st.l r1,12(sp)
	adds 8,sp,fp
	ld.l 0(r17),r18
	fld.l 20(r17),f16	// f16 = patValue
	orh ha%_framebytewidth,r0,r31
	ld.l l%_framebytewidth(r31),r25
	fmov.ss f16,f17		// f17 = patValue
	ld.s 6(r18),r20
	ld.s 4(r16),r17
	addu r17,r20,r20
	ld.l 12(r16),r22
	shl 16,r20,r20
	shra 16,r20,r20
	ixfr r25,f8
	ixfr r20,f12
	ixfr r21,f13
	fmlow.dd f8,f12,f10
	fxfr f10,r20
	fxfr f11,r21
	orh ha%_framebase,r0,r31
	ld.l l%_framebase(r31),r19
	addu r19,r20,r20
	ld.l 0(r18),r18
	shl 2,r18,r18
	addu r18,r20,r20
	ld.s 6(r16),r16
	adds -1,r0,r26		// r26 = -1 (loop decrement)
	subu r16,r17,r21
	bte r0,r21,.L2		// Check for lines == 0
	adds -1,r21,r21
.L10:
	ld.s 0(r22),r19
	addu 2,r22,r22
	bte r0,r19,.L3		// Check for pairs == 0
	adds -1,r19,r19
.L9:
	ld.s 0(r22),r16		// r16 = xl
	ld.s 2(r22),r17		// r17 = xr
	addu 4,r22,r22
	shl 2,r16,r23
	addu r23,r20,r18	// r18 = destunit = destbase + xl
	subu r17,r16,r16	// r16 = xr - xl
	bte r16,r0,.L4		// Skip out completely if width = 0
	shr 3,r16,r31
	btne r31,r0,.fast	// Skip to 2-at-a-time loop if units > 7

//--------------------------   1-at-a-time runmark ------------------------
	adds -1,r16,r16		// r17 = xr - xl - 1 (for bla)
	bla r26,r16,.L6
	addu -4,r18,r18		// Prepare for autoinc fst
.L6:
	bla r26,r16,.L6
	fst.l f16,4(r18)++
	br .L4
	nop
//--------------------------   2-at-a-time runmark ------------------------
.fast:	
	shl 2,r16,r31
	adds -4,r31,r31		// r31 = offset to last pixel
	fst.l f16,r31(r18)	// draw last pixel

	and 7,r18,r0		
	bc .skip1		// draw odd pixel and increment to doubleword 
	fst.l f16,0(r18)
	adds 4,r18,r18
	adds -1,r16,r16
.skip1:
	shr 1,r16,r31		// r31 = # doubles to draw
	adds -1,r31,r31
	bla r26,r31,.bla1
	adds -8,r18,r18
.bla1:
	bla r26,r31,.bla1
	fst.d f16,8(r18)++
//-------------------------------------------------------------------------
.L4:
	adds -1,r19,r19
	bnc .L9
.L3:
	adds -1,r21,r21
	bnc.t .L10
	addu r25,r20,r20
.L2:
	ld.l 4(fp),r1
	ld.l 0(fp),fp
	bri r1
	addu 16,sp,sp
