	.text
	nop	// FIR - 4 pager check value
gcc_compiled.:
.data
	.align 4
_expand2to32:
	.long 255
	.long 1431655935
	.long -1431655681
	.long -1
	.align 8
_expand2to32d:
	.long 255
	.long 255
	.long 255
	.long 1431655935
	.long 255
	.long -1431655681
	.long 255
	.long -1

	.long 1431655935
	.long 255
	.long 1431655935
	.long 1431655935
	.long 1431655935
	.long -1431655681
	.long 1431655935
	.long -1

	.long -1431655681
	.long 255
	.long -1431655681
	.long 1431655935
	.long -1431655681
	.long -1431655681
	.long -1431655681
	.long -1

	.long -1
	.long 255
	.long -1
	.long 1431655935
	.long -1
	.long -1431655681
	.long -1
	.long -1
	.align 4
_expand2to32a:
	.long 0
	.long 85
	.long 170
	.long 255
	.long 0
	.long 1431655765
	.long 1431655850
	.long 1431655935
	.long 0
	.long 1431655765
	.long -1431655766
	.long -1431655681
	.long 0
	.long 1431655765
	.long -1431655766
	.long -1
_expandmpto32:
	.long -1
	.long -1431655681
	.long 1431655935
	.long 255
	.align 8
_expandmpto32d:
	.long -1
	.long -1
	.long -1
	.long -1431655681
	.long -1
	.long 1431655935
	.long -1
	.long 255

	.long -1431655681
	.long -1
	.long -1431655681
	.long -1431655681
	.long -1431655681
	.long 1431655935
	.long -1431655681
	.long 255

	.long 1431655935
	.long -1
	.long 1431655935
	.long -1431655681
	.long 1431655935
	.long 1431655935
	.long 1431655935
	.long 255

	.long 255
	.long -1
	.long 255
	.long -1431655681
	.long 255
	.long 1431655935
	.long 255
	.long 255
	.align 4
_expandmpto32a:
	.long 0
	.long 1431655765
	.long -1431655766
	.long -1
	.long 0
	.long 85
	.long 1431655850
	.long -1431655681
	.long 0
	.long 85
	.long 170
	.long 1431655935
	.long 0
	.long 85
	.long 170
	.long 255
.text
	.align 4
.globl _BM38Convert2to32
_BM38Convert2to32:
	adds -32,sp,sp
	st.l fp,24(sp)
	st.l r1,28(sp)
	adds 24,sp,fp
	st.l r4,0(sp)
	st.l r5,4(sp)
	st.l r6,8(sp)
	st.l r7,12(sp)
	st.l r8,16(sp)
	st.l r9,20(sp)
	mov r28,r22
	mov r16,r24
	mov r17,r23
	ld.s 2(r22),r16
	ld.s 0(r22),r17
	subu r16,r17,r4
	ld.s 6(r22),r16
	ld.s 4(r22),r17
	subu r16,r17,r29
	ld.s 8(r22),r16
	ld.s 4(r23),r17
	subu r16,r17,r16
	ld.s 12(r22),r18
	ld.s 8(r23),r17
	shl 1,r16,r30
	and 31,r30,r30
	ld.l 24(r23),r20
	subu r18,r17,r18
	shra 2,r20,r20
	ixfr r18,f8
	ixfr r19,f9
	ixfr r20,f10
	ixfr r21,f11
	fmlow.dd f10,f8,f10
	fxfr f10,r18
	fxfr f11,r19
	shra 4,r16,r16
	addu r16,r18,r21
	shl 2,r21,r17
	ld.l 32(r23),r16
	addu r16,r17,r25
	ld.l 36(r23),r26
	bte r26,r0,.L2
	addu r17,r26,r26
.L2:
	ld.l 24(r23),r21
	ld.s 12(r23),r31	// r31 = sbm->type
	ld.s 0(r22),r16
	ld.s 4(r24),r17
	subu r16,r17,r16
	ld.s 4(r22),r18
	ld.s 8(r24),r19
	ld.l 24(r24),r17
	subu r18,r19,r6
	shra 2,r17,r8
	ixfr r6,f8
	ixfr r7,f9
	ixfr r8,f10
	ixfr r9,f11
	fmlow.dd f10,f8,f8
	fxfr f8,r18
	fxfr f9,r19
	addu r16,r18,r27
	shl 2,r27,r16
	ld.l 32(r24),r18
	addu r18,r16,r23
	shl 2,r4,r16
	subu r17,r16,r27
	or 30,r0,r16
	subu r16,r30,r30
	btne r26,r0,.L3
	bte r31,r0,.mp1			// if (mp != NX_OTHERBMTYPE) {
	or l%_expand2to32,r0,r17	//
	orh h%_expand2to32,r17,r17 	//     r17 = expand2to32
	or l%_expand2to32d,r0,r6	//
	br .mp2
	orh h%_expand2to32d,r6,r6 	//     r6 = expand2to32d
.mp1:					// } else {
	or l%_expandmpto32,r0,r17	// 
	orh h%_expandmpto32,r17,r17	//     r17 = expandmpto32
	or l%_expandmpto32d,r0,r6	// 
	orh h%_expandmpto32d,r6,r6	//     r6 = expandmpto32d
.mp2:					// }
	adds -4,r23,r23		// Compensate for fst.l++
	or l%1431655765,r0,r24	// Setup constant grays in registers
	orh h%1431655765,r24,r24 // r0 = 0x00000000	r24 = 0x55555555
	subs -1,r24,r26		// r26 = 0xaaaaaaaa	r31 = 0xffffffff
	adds -1,r0,r31		// Initialize 1-at-a-time bla decrement
.loop1:
	adds -1,r4,r20		// Compute count for 1-at-a-time bla
	ld.l 0(r25),r22
	addu 4,r25,r19
	bla r31,r20,.bla1
	mov r30,r18
.bla1:
	shr r18,r22,r16
	and 3,r16,r16
	shl 2,r16,r16
	fld.l r16(r17),f16
	bte r18,r0,.newsrc1
	adds -2,r18,r18
	bla r31,r20,.bla1
	fst.l f16,4(r23)++
	br .end1
	nop
.const16:
	shl 3,r22,r16
	and 0x78,r16,r16
	fld.d r16(r6),f16	// Lookup the correct double to store
	andnot 7,r23,r16	// r16 = rounded down version
	fst.l f16,4(r23)	// first pixel
	fst.d f16,8(r16)++	// 7 doubles for 14 middle pixels
	fst.d f16,8(r16)++			
	adds -16,r20,r20	// Decrement loop count by 16
	fst.d f16,8(r16)++			
	fst.d f16,8(r16)++			
	fst.d f16,8(r16)++			
	fst.d f16,8(r16)++			
	fst.d f16,8(r16)++			
	fst.l f16,60(r23)++	// 2nd to last pixel
.newsrc1:
	fst.l f16,4(r23)++	// last pixel
	ld.l 0(r19),r22
	addu 4,r19,r19
	adds -16,r20,r0
	shl 29,r23,r5
	bc .abort		// If count < 16, abort.
	bte r22,r0, .const16	// if (src == 0x00000000)
	bte r22,r24,.const16	// if (src == 0x55555555)
	bte r22,r31,.const16	// if (src == 0xffffffff)
	bte r22,r26,.const16	// if (src == 0xaaaaaaaa)
	bte r5,r0,.unaligned	// if (double aligned) {

	shr 25,r22,r16		// double aligned case
	and 0x78,r16,r16
	fld.d r16(r6),f16
	shr 21,r22,r16
	adds -4,r23,r23
	fst.d f16,8(r23)++	// pixel 0-1
	and 0x78,r16,r16
	fld.d r16(r6),f16
	shr 17,r22,r16
	fst.d f16,8(r23)++	// pixel 2-3
	and 0x78,r16,r16
	fld.d r16(r6),f16
	shr 13,r22,r16
	fst.d f16,8(r23)++	// pixel 4-5
	and 0x78,r16,r16
	fld.d r16(r6),f16
	shr 9,r22,r16
	fst.d f16,8(r23)++	// pixel 6-7
	and 0x78,r16,r16
	fld.d r16(r6),f16
	shr 5,r22,r16
	fst.d f16,8(r23)++	// pixel 8-9
	and 0x78,r16,r16
	fld.d r16(r6),f16
	shr 1,r22,r16
	fst.d f16,8(r23)++	// pixel 10-11
	and 0x78,r16,r16
	fld.d r16(r6),f16
	shl 3,r22,r16
	fst.d f16,8(r23)++	// pixel 12-13
	adds -16,r20,r20	// Decrement loop count by 16
	and 0x78,r16,r16
	fld.d r16(r6),f16
	br .newsrc1		// pixel 15 is done at top of loop
	fst.l f17,8(r23)++	// pixel 14

	nop			// double align this loop
.unaligned:
	d.fnop			// double unaligned case
	shr 25,r22,r16
	d.fnop			; and 0x78,r16,r16
	d.fnop			; fld.d r16(r6),f16
	d.fnop			; shr 21,r22,r16
	d.fmov.ss f16,f19	; fst.l f17,4(r23)	// pixel 0
	d.fnop			; and 0x78,r16,r16
	d.fnop			; fld.d r16(r6),f16
	d.fnop			; shr 17,r22,r16
	d.fmov.ss f17,f18	; and 0x78,r16,r16
	d.fnop			; fst.d f18,8(r23)++	// pixel 1-2
	d.fmov.ss f16,f19	; fld.d r16(r6),f16
	d.fnop			; shr 13,r22,r16
	d.fmov.ss f17,f18	; and 0x78,r16,r16
	d.fnop			; fst.d f18,8(r23)++	// pixel 3-4
	d.fmov.ss f16,f19	; fld.d r16(r6),f16
	d.fnop			; shr 9,r22,r16
	d.fmov.ss f17,f18	; and 0x78,r16,r16
	d.fnop			; fst.d f18,8(r23)++	// pixel 5-6
	d.fmov.ss f16,f19	; fld.d r16(r6),f16
	d.fnop			; shr 5,r22,r16
	d.fmov.ss f17,f18	; and 0x78,r16,r16
	d.fnop			; fst.d f18,8(r23)++	// pixel 7-8
	d.fmov.ss f16,f19	; fld.d r16(r6),f16
	d.fnop			; shr 1,r22,r16
	d.fmov.ss f17,f18	; and 0x78,r16,r16
	d.fnop			; fst.d f18,8(r23)++	// pixel 9-10
	d.fmov.ss f16,f19	; fld.d r16(r6),f16
	d.fnop			; shl 3,r22,r16
	d.fmov.ss f17,f18	; and 0x78,r16,r16
	d.fnop			; fst.d f18,8(r23)++	// pixel 11-12
	d.fmov.ss f16,f19	; fld.d r16(r6),f16
	  fnop			; adds -16,r20,r20	// Dec loop count by 16
	  fmov.ss f17,f18	; nop
	fst.d f18,8(r23)++				// pixel 13-14
	br .newsrc1					// pixel 15 done at top
	adds 4,r23,r23					// adjust dp back by 4

.abort:
	bla r31,r20,.bla1
	or 30,r0,r18
.end1:	
	addu r21,r25,r25
	addu r27,r23,r23
	addu -1,r29,r29
	btne r29,r0,.loop1
	br .L11
	nop

//--------------------- 2->32 conversion with alpha ---------------------
.L3:
	bte r31,r0,.mp3			// if (mp != NX_OTHERBMTYPE) {
	or l%_expand2to32a,r0,r6
	br .mp4
	orh h%_expand2to32a,r6,r6 	// } else {
.mp3:
	or l%_expandmpto32a,r0,r6
	orh h%_expandmpto32a,r6,r6	// }
.mp4:
	adds -4,r23,r23		// Compensate for fst.l++
	mov r21,r28
	adds -1,r0,r31		// Initialize 1-at-a-time bla decrement
.loop2:
	adds -1,r4,r20		// Compute count for 1-at-a-time bla
	ld.l 0(r25),r22
	addu 4,r25,r19
	ld.l 0(r26),r24
	addu 4,r26,r21
	bla r31,r20,.bla2
	mov r30,r18
.bla2:
	shr r18,r22,r16		// r17 = ...???????dd
	and 3,r16,r16		// r17 = ...0000000dd
	shl 4,r16,r16		// r16 = ...000dd0000
	shr r18,r24,r17		// r17 = ...???????aa
	and 3,r17,r17		// r17 = ...0000000aa
	shl 2,r17,r17		// r17 = ...00000aa00
	or r16,r17,r16		// r16 = ...000ddaa00
	fld.l r16(r6),f16	// f16 = 0xddddddaa
	bte r18,r0,.newsrc2
	adds -2,r18,r18
	bla r31,r20,.bla2
	fst.l f16,4(r23)++
	br .end2
	nop
.newsrc2:
	ld.l 0(r19),r22
	or 30,r0,r18
	addu 4,r19,r19
	ld.l 0(r21),r24
	addu 4,r21,r21
	bla r31,r20,.bla2
	fst.l f16,4(r23)++
.end2:	
	addu r28,r25,r25
	addu r28,r26,r26
	addu r27,r23,r23
	addu -1,r29,r29
	btne r29,r0,.loop2
.L11:
	ld.l -24(fp),r4
	ld.l -20(fp),r5
	ld.l -16(fp),r6
	ld.l -12(fp),r7
	ld.l -8(fp),r8
	ld.l -4(fp),r9
	ld.l 4(fp),r1
	ld.l 0(fp),fp
	bri r1
	addu 32,sp,sp
