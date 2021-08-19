	.text
	nop	// FIR - 4 pager check value
gcc_compiled.:
.text
	.align 4
.globl _BMSover38
_BMSover38:
	adds -48,sp,sp
	st.l fp,40(sp)
	st.l r1,44(sp)
	adds 40,sp,fp
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
	ld.l 8(r16),r26
	ld.l 44(r16),r8
	ld.l 36(r16),r20
	ld.l 4(r16),r30
	ld.l 12(r16),r17
	ld.l 16(r16),r24
	ld.l 24(r16),r9
	ld.l 20(r16),r29
	ld.l 40(r16),r28
	ld.l 28(r16),r25
	ld.c	psr,r31
	andnoth 0xc0,r31,r31
	st.c	r31,psr		// set PS to be 8 bits	
	shl 2,r8,r8
	shl 2,r9,r9
	shl 2,r28,r28
	shl 2,r29,r29
	or l%-16711936,r0,r27
	orh h%-16711936,r27,r27
	or l%16711935,r0,r23
	orh h%16711935,r23,r23
	btne r17,r0,.L2

//	mov r20,r31		// r31 = dp
//	shl 2,r30,r21		// r21 = width * 4
//	addu r8,r21,r21		// r21 = rowbytes
//	mov r26,r25		// Compute count for scanline loop
//.hack:
//	fld.l 0(r31),f16	// read
//	adds -1,r25,r25		// height--
//	fst.l f16,0(r31)	// and write all the pages...
//	addu r21,r31,r31	// dp += rowbytes
//	btne r25,r0,.hack	// loop to hack.
//	nop

	btne 4,r29,.sover2	// if we are going backward, do it one-at-a-time
	shr 3,r30,r31
	btne r31,r0,.sover3	// if width > 7, do it 2-at-a-time 
//-------------------------  1-at-a-time Bitmap Sover -------------------------
.sover2:
	ixfr r23,f24		// f24 = 0x00ff00ff
	adds -1,r0,r31		// Initialize 1-at-a-time bla decrement
	fmov.ss f24,f25		// f25 = 0x00ff00ff
	subu r24,r29,r24	// predecrement pointers for f{ld,st}++
	subu r20,r28,r20
	subs 0,r28,r4		// r4 = -ro->dstInc
.loop2:
	adds -1,r30,r21		// Compute count for 1-at-a-time bla
	fld.l r28(r20)++,f20	// f20 = d = *++dp
	ld.l r29(r24),r25	// r25 = s = *sp
	bla r31,r21,.skip2
	fld.l r29(r24)++,f22	// f22 = s = *++sp
.skip2:
	nop
	d.fnop
	nop
	d.form	 f0,f0     ; br .begin2 // M = 0
	d.faddp	 f0,f20,f0 ; nop 	// M = R0B0R0B0 Start after store

.blaopaque2:
	d.fnop	; ld.l r29(r24),r25	// r25 = s = *sp
	d.fnop	; addu r29,r24,r24	// sp++
	d.fnop	; addu r28,r20,r20	// dp++
.opaque2:
	d.fnop	; subu -1,r25,r16
	d.fnop	; and 255,r16,r22
	d.fnop	; btne r0,r22,.other2	// if (alpha != 255) doother
	d.fnop	; bla r31,r21,.blaopaque2
	d.fnop	; st.l r25,0(r20)	// *(dp-1) = s
	fnop	; nop			// can't have a br here...
	fnop	; br .end2
	fnop	; nop

.other2:
	d.form f0,f0	  ; fld.l 0(r20),f20	// f20 = d = *dp
	d.fnop		  ; br .begin2
	d.faddp f0,f20,f0 ; fld.l 0(r24),f22	// f22 = s = *sp

.blaclear2:
	d.fnop	; ld.l r29(r24),r25	// r25 = s = *sp
	d.fnop	; addu r29,r24,r24	// sp++
	d.fnop	; addu r28,r20,r20	// dp++
.clear2:
	d.fnop	; and 255,r25,r22
	d.fnop	; btne r0,r22,.other2	// if (alpha != 0) doother
	d.fnop	; bla r31,r21,.blaclear2
	d.fnop	; nop
	fnop	; nop			// can't have a br here...
	fnop	; br .end2
	fnop	; nop

.bla2:
// 	d.faddp	 f0,f20,f0	; fst.l f16,r4(r20)	// M   = R0B0R0B0      *(dp-4) = s
	d.fnop			; fst.l f16,r4(r20)	// 		       *(dp-4) = s
	d.faddp	 f0,f20,f0	; nop			//  M  = R0B0R0B0     
.begin2:
	d.form	 f0,f16		; and 255,r25,r22	// f16 = R0B0R0B0 M=0  r22 = alpha
	d.faddp	 f0,f20,f0	; bte r0,r25,.clear2	//  M  = R0B0R0B0      if (ENTIRE PIXEL == 0)
	d.fisub.dd f20,f16,f18	; subu 255,r22,r22	// f18 = 0G0A0G0A      r22 = NEGALPHA(s)
	d.faddp	 f0,f0,f0	; ixfr r22,f12		//  M  = 0R0B0R0B      f12 = NEGALPHA(s)
	d.form	 f0,f16		; bte r0,r22,.opaque2	// f16 = 0R0B0R0B M=0  if (PIXEL ALPHA == 0xff)
	d.fmov.dd f22,f14	; bte r31,r21,.cleanup2	// f14 = RGBARGBA S    Leave loop
	d.fmlow.dd f12,f18,f18	; fld.l r28(r20)++,f20	// f18 = GgAaGgAa      f20 = d = *++dp
	d.fmlow.dd f12,f16,f16	; ld.l r29(r24),r25	// f16 = RrBbRrBb      r25 = s = *(sp+1)
	d.faddp	 f24,f18,f0	; fld.l r29(r24)++,f22	//  M  = G0A0G0A0      f22 = s = *++sp
	d.faddp	 f24,f16,f0	; nop			//  M  = RGBARGBA
	d.form	 f0,f16		; bla r31,r21,.bla2	// f16 = RGBARGBA D
	d.fiadd.dd f14,f16,f16	; nop			// f16 = RGBARGBA S+D
.cleanup2:				// Always reached from the bte .cleanup2
	fmlow.dd f12,f18,f18	; nop	// f18 = GgAaGgAa
	fmlow.dd f12,f16,f16	; nop	// f16 = RrBbRrBb
	faddp	 f24,f18,f0		// M   = G0A0G0A0
	faddp	 f24,f16,f0		// M   = RGBARGBA
	form	 f0,f16			// f16 = RGBARGBA D
	fiadd.dd f14,f16,f16		// f16 = RGBARGBA S+D
	fst.l f16,0(r20)		// *dp = s
.end2:
	addu -1,r26,r26
	addu r8,r20,r20
	addu r9,r24,r24
	btne r26,r0,.loop2
	br .L11
	nop

//---------------------    2-at-a-time Bitmap Sover --------------------
.sover3:
	ixfr r23,f24		//
	orh 0xffff,r0,r27	// r27 = 0xffff0000
	ixfr r27,f30		//
	adds -1,r0,r31		// Initialize 1-at-a-time bla decrement
	ixfr r31,f8		//
	fmov.ss f24,f25		// f24 = 0F0F0F0F
	fmov.ss f30,f31		// f30 = FF00FF00
	fmov.ss f8,f9		// f8  = FFFFFFFF
.loop3:
	and 7,r20,r21
	mov r30,r29		// r29 = count
	bte r21,r0,.doubles3	// if (dp not double aligned) { do one pixel sover
	fld.l 0(r20),f20	// f20 = d = *dp
	ld.l 0(r24),r25		// r25 = s = *sp
	fld.l 0(r24),f22	// f22 = s = *sp
	and 255,r25,r22		// r22 = alpha
	subu 255,r22,r22	// r22 = NEGALPHA(s)
	ixfr r22,f12		// f12 = NEGALPHA(s)
	faddp	 f0,f20,f0	// M   = R0B0R0B0
	form	 f0,f16	 	// f16 = R0B0R0B0 M=0
	faddp	 f0,f20,f0	// M   = R0B0R0B0 
	fisub.dd f20,f16,f18	// f18 = 0G0A0G0A
	faddp	 f0,f0,f0	// M   = 0R0B0R0B
	form	 f0,f16		// f16 = 0R0B0R0B M=0
	fmlow.dd f12,f18,f18	// f18 = GgAaGgAa
	fmlow.dd f12,f16,f16	// f16 = RrBbRrBb
	faddp	 f24,f18,f0	// M   = G0A0G0A0
	faddp	 f24,f16,f0	// M   = RGBARGBA
	form	 f0,f16		// f16 = RGBARGBA D
	fiadd.dd f22,f16,f16	// f16 = RGBARGBA S+D
	fst.l f16,0(r20)	// *dp = s
	adds 4,r20,r20		// dp++
	adds 4,r24,r24		// sp++
	adds -1,r29,r29		// decrement count
.doubles3:
	shr 1,r29,r21		// Two pixel sover, divide count by 2
	adds -1,r21,r21		// Compute count for 1-at-a-time bla
	fld.d 0(r20),f20	// f20 = d = *dp
	ld.l 0(r24),r28		// r28 = s = *sp
	bla r31,r21,.skip3
	ld.l 4(r24),r25		// r25 = s = *sp+1
.skip3:
	fld.l 0(r24),f23	// f23 = s = *sp
	nop
	d.fnop
	fld.l 4(r24),f22	// f22 = s = *sp+1
	d.form	 f0,f0     ; br .begin3 		// M   = 0
	d.faddp	 f0,f20,f0 ; and 255,r25,r4	 	// M   = R0B0R0B0      r4 = alpha1

.blaopaque3:
	d.pfeq.dd f14,f8,f0	; bte r31,r21,.other3	// 		       if (count == -1)    Leave
	d.fnop			; bnc .other3		//		       if (f14!=FFFFFFFF)  Leave
.opaque3:
	d.fmov.dd f22,f12	; fld.l 8(r24)++,f23	// f12 = f22 = s       f23 = S0
	d.faddp f0,f30,f0	; fld.l 4(r24),f22	// M  = F?0?F?0?       f22 = S1
	d.faddp f0,f8,f0	; fst.d f12,0(r20)	// M = FFF0FFF0	       *dp = f12
	d.fnop			; bla r31,r21,.blaopaque3     
	d.form f22,f14		; adds 8,r20,r20	// f14 = FFFAFFFA      dp += 8

.blaclear3:
	d.pfeq.dd f22,f0,f0	; bte r31,r21,.other3	// if (count == -1)    Leave loop
	d.fnop			; bnc .other3		// if (f22!=00000000)  Leave loop
	d.fnop			; fld.l 8(r24)++,f23	// 		       f23 = S0	
	d.fnop			; adds 8,r20,r20	// 		       r20 = dp += 8
	d.fnop			; bla r31,r21,.blaclear3
	d.fnop			; fld.l 4(r24),f22	// 		       f22 = S1

.other3:
	d.fnop		  	; ld.l 0(r24),r28	// 		       r28 = S0 = *(sp+1)
	d.fnop		  	; ld.l 4(r24),r25	// 		       r25 = S1 = *(sp+2)
	d.form f0,f0	  	; fld.d 0(r20),f20	//  M  = 0	       f20 = d = *dp
	d.fnop		  	; br .begin3
	d.faddp f0,f20,f0 	; and 255,r25,r4	// f22 = s = *sp       r4 = alpha1

.bla3:
	d.fnop			; fst.d f16,-8(r20)	// 		       *(dp-4) = s
	d.faddp	 f0,f20,f0	; nop			//  M  = R0B0R0B0     
.begin3:
	d.form	 f0,f16		; or r28,r25,r25	// f16 = R0B0R0B0 M=0  r25 = s0 | s1
	d.faddp	 f0,f20,f0	; and 255,r28,r28	//  M  = R0B0R0B0      r28 = alpha0
	d.fisub.dd f20,f16,f18	; subu 255,r4,r4	// f18 = 0G0A0G0A D01  r4  = ~alpha1
	d.faddp	 f0,f0,f0	; ixfr r4,f10		//  M  = 0R0B0R0B      f10 = ~alpha1
	d.form	 f0,f16		; subu 255,r28,r28	// f16 = 0R0B0R0B D01  r28 = ~alpha0
	d.fmov.dd f22,f14	; ixfr r28,f12		// f14 = RGBARGBA S01  f12 = ~alpha0
	d.fmov.ss f19,f28	; bte r31,r21,.cleanup3	// f28 = ????0G0A D1   Leave loop
	d.fmov.ss f17,f26	; bte r0,r25,.blaclear3	// f26 = ????0R0B D1   if (ENTIRE PIXELS == 0)
	d.fmlow.dd f10,f18,f18	; or r28,r4,r4		// f18 = ????GgAa D0   r4  = ~alpha0|~alpha1
	d.fmlow.dd f10,f16,f16	; bte r0,r4,.opaque3	// f16 = ????RrBb D0   if (PIXEL ALPHAS == 0xff)
	d.fmlow.dd f12,f28,f28	; fld.d 8(r20)++,f20	// f28 = ????GgAa D1   f20 = D01 = *++dp
	d.fmlow.dd f12,f26,f26	; ld.l 8(r24),r28	// f26 = ????RrBb D1   r28 = S0 = *(sp+1)
	d.fmov.ss f28,f19	; ld.l 12(r24),r25	// f18 = GgAaGgAa D01  r25 = S1 = *(sp+2)
	d.fmov.ss f26,f17	; nop			// f16 = RrBbRrBb D01  
	d.faddp	 f24,f18,f0	; fld.l 8(r24)++,f23	//  M  = G0A0G0A0      f23 = S0 = *++sp
	d.faddp	 f24,f16,f0	; fld.l 4(r24),f22	//  M  = RGBARGBA      f22 = S1 = *++sp
	d.form	 f0,f16		; bla r31,r21,.bla3	// f16 = RGBARGBA D
	d.fiadd.dd f14,f16,f16	; and 255,r25,r4	// f16 = RGBARGBA S+D  r4 = alpha1
.cleanup3:				// Always reached from the bte .cleanup2
	fmov.ss f17,f26		; nop	// f26 = ????0R0B D1
	fmlow.dd f10,f18,f18	; nop	// f18 = ????GgAa D0
	fmlow.dd f10,f16,f16		// f16 = ????RrBb D0
	fmlow.dd f12,f28,f28		// f28 = ????GgAa D1
	fmlow.dd f12,f26,f26		// f26 = ????RrBb D1
	fmov.ss f28,f19			// f18 = GgAaGgAa D01
	fmov.ss f26,f17			// f16 = RrBbRrBb D01
	faddp	 f24,f18,f0		// M   = G0A0G0A0
	faddp	 f24,f16,f0		// M   = RGBARGBA
	form	 f0,f16			// f16 = RGBARGBA D
	fiadd.dd f14,f16,f16		// f16 = RGBARGBA S+D
	fst.d f16,0(r20)		// *dp = s

	adds 8,r20,r20		// r20 = dp += 8	to correct pointers for rowbytes
	adds 8,r24,r24		// r24 = sp += 8

	and 1,r29,r29
	bte r29,r0,.end3	// if (there is a final odd pixel) {
	fld.l 0(r20),f20	// f20 = d = *dp
	ld.l 0(r24),r25		// r25 = s = *sp
	fld.l 0(r24),f22	// f22 = s = *sp
	and 255,r25,r22		// r22 = alpha
	subu 255,r22,r22	// r22 = NEGALPHA(s)
	ixfr r22,f12		// f12 = NEGALPHA(s)
	faddp	 f0,f20,f0	// M   = R0B0R0B0
	form	 f0,f16	 	// f16 = R0B0R0B0 M=0
	faddp	 f0,f20,f0	// M   = R0B0R0B0 
	fisub.dd f20,f16,f18	// f18 = 0G0A0G0A
	faddp	 f0,f0,f0	// M   = 0R0B0R0B
	form	 f0,f16		// f16 = 0R0B0R0B M=0
	fmlow.dd f12,f18,f18	// f18 = GgAaGgAa
	fmlow.dd f12,f16,f16	// f16 = RrBbRrBb
	faddp	 f24,f18,f0	// M   = G0A0G0A0
	faddp	 f24,f16,f0	// M   = RGBARGBA
	form	 f0,f16		// f16 = RGBARGBA D
	fiadd.dd f22,f16,f16	// f16 = RGBARGBA S+D
	fst.l f16,0(r20)	// *dp = s
	adds 4,r20,r20		// dp++
	adds 4,r24,r24		// sp++
.end3:
	addu -1,r26,r26
	addu r8,r20,r20
	addu r9,r24,r24
	btne r26,r0,.loop3
	br .L11
	nop
//-------------------------    Constant Sover -------------------------
.L2:
	subu -1,r25,r22
	and 255,r22,r22
	ixfr r22,f8		// f8 = r22 = sa = NEGALPHA(s)
	adds -1,r0,r31		// Initialize 1-at-a-time bla decrement
.loop1:
	adds -1,r30,r21		// Compute count for 1-at-a-time bla
	bla r31,r21,.bla1
	nop
.bla1:
	ld.l 0(r20),r18		// r18 = d = *dp, r20 = dp
	and r27,r18,r16		// r16 = d & 0xff00ff00
	shr 8,r16,r16
	ixfr r16,f16		// f16  = r16 = 0x00rr00bb
	and r23,r18,r6
	ixfr r6,f18		// f18  = r6  = 0x00gg00aa
	fmlow.dd f8,f16,f16
	fmlow.dd f8,f18,f18
	fxfr f16,r16		// r16  = 0xRRrrBBbb
	fxfr f18,r18		// r18  = 0xGGggAAaa
	addu r23,r16,r16	// r16 += 0x00ff00ff
	addu r23,r18,r18	// r18 += 0x00ff00ff
	shr 8,r18,r18		// r18  = 0x00GGggAA
	and r27,r16,r16		// r16  = 0xRR00BB00
	and r23,r18,r18		// r18  = 0x00GG00AA
	or r18,r16,r16		// r16  = 0xRRGGBBAA
	addu r16,r25,r16	// r16 += s
	st.l r16,0(r20)		// *dp++ = s + MUL(d, sa)
	bla r31,r21,.bla1
	addu 4,r20,r20

	addu -1,r26,r26
	addu r8,r20,r20
	btne r26,r0,.loop1
.L11:
	ld.l -40(fp),r4
	ld.l -36(fp),r5
	ld.l -32(fp),r6
	ld.l -28(fp),r7
	ld.l -24(fp),r8
	ld.l -20(fp),r9
	ld.l -16(fp),r10
	ld.l -12(fp),r11
	ld.l -8(fp),r12
	ld.l -4(fp),r13
	ld.l 4(fp),r1
	ld.l 0(fp),fp
	bri r1
	addu 48,sp,sp
