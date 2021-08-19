	.text
	nop	// FIR - 4 pager check value
gcc_compiled.:
.text
	.align 4
.globl _ConstantTrapsMark32
_ConstantTrapsMark32:
	adds -32,sp,sp
	st.l fp,24(sp)
	st.l r1,28(sp)
	adds 24,sp,fp
	st.l r4,0(sp)
	st.l r5,4(sp)
	ld.l 0(r18),r4
	mov r16,r21
	mov r17,r30
	ld.l 0(r4),r5		// r5 = info->offset.x
	ld.l 4(r4),r4		// r4 = info->offset.y
	shl 2,r5,r5
	fld.l 20(r18),f16	// f16 = patValue
	orh ha%_framebytewidth,r0,r31
	ld.l l%_framebytewidth(r31),r25
	fmov.ss f16,f17		// f17 = patValue
	addu -28,r21,r21
	ixfr r25,f10
	bte r0,r30,.L29
.L34:
	addu 28,r21,r21
	ld.s 0(r21),r23
	ld.s 2(r21),r16
	subu r16,r23,r20	// lines = t->y.g - y;
	addu r4,r23,r23		// y += info->offset.y
	ixfr r23,f8	// destbase = framebase + y*fbw + info->offset.x
	fmlow.dd f10,f8,f12
	subs 0,r20,r0
	bnc .L2			// if (lines <= 0) continue;
	fxfr f12,r16
	orh ha%_framebase,r0,r31
	ld.l l%_framebase(r31),r18
	addu r5,r16,r16
	addu r18,r16,r18	// r18 = destbase

	ld.s 16(r21),r22	// r22 = t->g.xl = xg
	ld.s 18(r21),r23	// r23 = t->g.xg
	shl 16,r22,r28		// r28 = gx  = xg << 16
	mov r0,r24		// r24 = gdx = 0
	bte r23,r22,.L7		// if (t->g.xl != t->g.xg) {
	ld.l 20(r21),r28
	ld.l 24(r21),r24
.L7:				// }
	ld.s 4(r21),r19		// r19 = t->l.xl = xl
	ld.s 6(r21),r26		// r26 = t->l.xg
	shl 16,r19,r27		// r27 = lx  = xl << 16
	mov r0,r29		// r29 = ldx = 0
	bte r26,r19,.L8		// if (t->l.xl != t->l.xg) {
	ld.l 8(r21),r27
	br .L9
	ld.l 12(r21),r29
.L8:				// }
	btne r23,r22,.L9	// if (t->l.xl == t->l.xg) { /* rectangle */
	subu r22,r19,r22	// r22 = xg -= xl;
	bte r22,r0,.L2		// if(xg == 0) continue;
	shl 2,r19,r16
	addu r18,r16,r16	// r16 = destunit = destbase + xl;

	adds -2,r0,r26		// Initialize 2-at-a-time bla decrement
	shl 2,r22,r17		//
	adds -4,r17,r17		// Compute offset to end pixel
	and 7,r16,r0		// Test for starting on odd pixel
	bc .loop0
	adds -1,r22,r22		// Started on odd pixel, so decrement width
.loop0:
	adds -2,r22,r19		// Compute count for 2-at-a-time bla
	fst.l f16,0(r16)	// Write starting pixel
	fst.l f16,r17(r16)	// Write ending pixel
	bc .skip0		// If width < 2, skip 2-at-a-time bla
	adds -4,r16,r31
	bla r26,r19,.bla0
	andnot 7,r31,r31	// Round off start to correct doubleword
.bla0:
	bla r26,r19,.bla0	// Do middle doublewords
	fst.d f16,8(r31)++
.skip0:
	addu -1,r20,r20
	addu r25,r16,r16	// Increment dest ptr to next scanline
	btne r20,r0,.loop0
	br .L2
	nop			// } else { /* Trapezoid */
.L9:
	adds -1,r0,r26		// r26 = -1 (loop decrement)
.L23:
	subu r22,r19,r17	// r17 = units = xg - xl;
	bte r17,r0,.L28		// Skip out completely if units == 0
	shl 2,r19,r16
	addu r16,r18,r16	// r16 = destunit = destbase + xl;
	shr 3,r17,r31
	btne r31,r0,.fast	// Skip to 2-at-a-time loop if units > 7

//--------------------------   1-at-a-time trapmark ------------------------
	adds -1,r17,r17		// r17 = units - 1 (for bla)
	bla r26,r17,.L26
	addu -4,r16,r16		// Prepare for autoinc fst
.L26:
	bla r26,r17,.L26	// while (--units >= 0) *destunit++ = patValue;
	fst.l f16,4(r16)++
	br .L28
	nop
//--------------------------   2-at-a-time trapmark ------------------------
.fast:	
	shl 2,r17,r31
	adds -4,r31,r31		// r31 = offset to last pixel
	fst.l f16,r31(r16)	// draw last pixel

	and 7,r16,r0		
	bc .skip1		// draw odd pixel and increment to doubleword 
	fst.l f16,0(r16)
	adds 4,r16,r16
	adds -1,r17,r17
.skip1:
	shr 1,r17,r31		// r31 = # doubles to draw
	adds -1,r31,r31
	bla r26,r31,.bla1
	adds -8,r16,r16
.bla1:
	bla r26,r31,.bla1
	fst.d f16,8(r16)++
//-------------------------------------------------------------------------
.L28:
	addu -1,r20,r20
	bte 0,r20,.L2		// if (--lines == 0) break;
	bte 1,r20,.L30		// if (lines == 1) do last line
	shra 16,r27,r19		// xl = IntPart(lx);
	addu r29,r27,r27	// lx += ldx;
	shra 16,r28,r22		// xg = IntPart(gx);
	addu r24,r28,r28	// gx += gdx;
	br .L23
	addu r25,r18,r18	// destbase += fbw;
.L30:
	ld.s 6(r21),r19		// xl = t->l.xg;
	ld.s 18(r21),r22	// xg = t->g.xg;
	br .L23
	addu r25,r18,r18	// destbase += fbw;
.L2:
	addu -1,r30,r30
	btne 0,r30,.L34
.L29:
	ld.l -24(fp),r4
	ld.l -20(fp),r5
	ld.l 4(fp),r1
	ld.l 0(fp),fp
	bri r1
	addu 32,sp,sp
