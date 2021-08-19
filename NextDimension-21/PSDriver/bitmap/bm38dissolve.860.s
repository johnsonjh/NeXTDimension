	.text
	nop	// FIR - 4 pager check value
gcc_compiled.:
.text
	.align 4
.globl _BMDissolve38
_BMDissolve38:
	adds -64,sp,sp
	st.l fp,56(sp)
	st.l r1,60(sp)
	adds 56,sp,fp
	st.l r4,0(sp)
	st.l r5,4(sp)
	st.l r6,8(sp)
	st.l r7,12(sp)
	st.l r8,16(sp)
	st.l r9,20(sp)
	st.l r10,24(sp)
	st.l r11,28(sp)
	st.l r12,32(sp)
	st.l r13,36(sp)
	st.l r14,40(sp)
	st.l r15,44(sp)
	ld.l 8(r16),r28
	ld.l 44(r16),r15
	ld.l 36(r16),r23
	ld.l 4(r16),r14
	ld.l 16(r16),r25
	ld.l 24(r16),r5
	ld.l 20(r16),r4
	ld.l 40(r16),r30
	or l%-16711936,r0,r26
	orh h%-16711936,r26,r26
	or l%16711935,r0,r22
	orh h%16711935,r22,r22
	shl 2,r30,r30		// r30 = ro->dstInc
	shl 2,r4,r4		// r4  = ro->srcInc
	shl 2,r15,r15		// r15 = ro->dstRowBytes
	shl 2,r5,r5		// r5  = ro->srcRowBytes
	ld.l 52(r16),r16
	and 255,r16,r27
	ixfr r27,f8		// f8 = ro->delta
	subu 255,r27,r29
	ixfr r29,f10		// f10 = 255 - ro->delta
	subs 0,r30,r29		// r29 = -ro->dstInc

	ld.c	psr,r31
	andnoth 0xc0,r31,r31
	st.c	r31,psr		// set PS to be 8 bits
	
	ixfr	r22,f20		// f20 = 0x00ff00ff
	adds -1,r0,r31		// r31 = bla loop decrement
	subu r25,r4,r25		// predecrement pointers for f{ld,st}++
	subu r23,r30,r23	// dp+=di
.L9:
	adds -1,r14,r24			// r31 = bla loop counter
	fld.l r30(r23)++,f23		// f23 = d = *++dp
	fld.l r4(r25)++,f22		// f22 = s = *++sp
	ld.l 0(r23),r16			// r16 = d = *dp
	ld.l 0(r25),r17			// r17 = s = *dp
	and  r22,r16,r16		// r16 = 0G0A
	bla r31,r24,.skip		// initialize bla
	nop
.skip:
	d.faddp	 f0,f22,f0				// M   = R0B0R0B0
	br .begin					// Start loop after store instruction
	d.fnop			; nop
.bla:
	d.faddp	 f0,f22,f0	; fst.l f16,r29(r23)	// M   = R0B0R0B0      *(dp-4) = f16
.begin:	d.faddp	 f0,f0,f0	; ixfr r16,f12		// M   = 0R0B0R0B      f12 = 0G0A SRC
	d.form	 f0,f16		; and  r22,r17,r17	// f16 = 0R0B0R0B M=0  r17 = 0G0A DST
	d.fmov.ss f17,f18	; ixfr r17,f14		// f18 = 00000R0B DST  f14 = 0G0A DST
	d.fmlow.dd f8,f12,f12	; bte r31,r24,.cleanup	// f12 = ??AaGgAa SRC  Leave loop
	d.fmlow.dd f10,f14,f14	; fld.l r30(r23)++,f23	// f14 = 0000GgAa DST  f23 = d = *++dp
	d.fmlow.dd f8,f16,f16	; nop			// f16 = ??BbRrBb SRC
	d.fmlow.dd f10,f18,f18	; fld.l r4(r25)++,f22	// f18 = 0000RrBb DST  f22 = s = *++sp
	d.fiadd.dd f12,f14,f14	; nop			// f14 = 0000GgAa S+D
	d.fiadd.dd f16,f18,f18	; ld.l 0(r23),r17	// f18 = 0000RrBb S+D  r17 = d = *dp
	d.faddp	 f20,f14,f0	; ld.l 0(r25),r16	// M   = 0000G0A0      r16 = s = *sp
	d.faddp	 f20,f18,f0	; bla r31,r24,.bla	// M   = 0000RGBA
	d.form	 f0,f16		; and  r22,r16,r16	// f16 = 0000RGBA      r16 = 0G0A SRC

.cleanup:				// Always reached from the bte .cleanup
	fmlow.dd f10,f14,f14	; nop	//	f14 = 0000GgAa DST
	fmlow.dd f8,f16,f16	; nop	// 	f16 = ??BbRrBb	SRC
	fmlow.dd f10,f18,f18		// 	f18 = 0000RrBb	DST
	fiadd.dd f12,f14,f14		// f14 = 0000GgAa	SRC + DST
	fiadd.dd f16,f18,f18		// 	f18 = 0000RrBb	SRC + DST
	faddp	 f20,f14,f0		// M   = 0000G0A0
	faddp	 f20,f18,f0		// M   = 0000RGBA
	form	 f0,f16			// f16 = 0000RGBA
	fst.l f16,0(r23)		// *dp = f16
.skipclean:
	addu -1,r28,r28
	addu r15,r23,r23
	addu r5,r25,r25
	btne r28,r0,.L9

	ld.l -56(fp),r4
	ld.l -52(fp),r5
	ld.l -48(fp),r6
	ld.l -44(fp),r7
	ld.l -40(fp),r8
	ld.l -36(fp),r9
	ld.l -32(fp),r10
	ld.l -28(fp),r11
	ld.l -24(fp),r12
	ld.l -20(fp),r13
	ld.l -16(fp),r14
	ld.l -12(fp),r15
	ld.l 4(fp),r1
	ld.l 0(fp),fp
	bri r1
	addu 64,sp,sp
