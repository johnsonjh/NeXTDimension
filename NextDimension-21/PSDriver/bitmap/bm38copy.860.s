	.text
	nop	// FIR - 4 pager check value
gcc_compiled.:
.text
	.align 4
.globl _BMCopy38
_BMCopy38:
	adds -16,sp,sp
	st.l fp,8(sp)
	st.l r1,12(sp)
	adds 8,sp,fp
	mov r16,r17
	ld.l 0(r17),r16		// r16 = op	  = ro->op
	ld.l 4(r17),r27		// r27 = width	  = ro->width
	ld.l 8(r17),r21		// r21 = height   = ro->height
	ld.l 12(r17),r18	// r18 = srcType  = ro->srcType
	fld.l 28(r17),f16	// f16 = srcValue = ro->srcValue
	bte r27,r0,.L23		// if (width == 0) return;
	bte r21,r0,.L23		// if (height == 0) return;
	btne r16,r0,.L2		// if (op == CLEAR) {
	fmov.ss f0,f16		//     srcValue = 0
	or 1,r0,r18		//     srcType = SRCCONSTANT
.L2:				// }
	fmov.ss f16,f17		// f17 = srcValue
	ld.l 44(r17),r26	// r26 = dstRowBytes (really longs)
	ld.l 36(r17),r19	// r19 = dstPtr
	ld.l 56(r17),r24	// r24 = mask
	shl 2,r26,r26		// r26 = dstRowBytes (now in bytes)
	btne r18,r0,.L31	// if (srcType == SRCBITMAP) {
	ld.l 24(r17),r25	//   r25 = srcRowBytes (really longs)
	ld.l 64(r17),r16	//   r16 = ltor
	ld.l 16(r17),r20	//   r20 = srcPtr
	shl 2,r25,r25		//   r25 = srcRowBytes (now in bytes)
	or 4,r0,r22 
	btne r16,r0,.L4
	adds -4,r0,r22		//   r22 = inc = (ltor ? 4 : -4)
.L4:
	bte r24,r0,.L22
//---------------------- Masked Copyrect ----------------------------

	subu -1,r24,r23		//   r23 = ~mask
	adds -1,r0,r31		// Initialize 1-at-a-time bla decrement
	subu r19,r26,r19	// Compensate for early dstPtr increment
.loop1:
	adds -1,r27,r18		// Compute count for 1-at-a-time bla
	bla r31,r18,.bla1
	addu r26,r19,r19	// Increment dstPtr to next scanline
.bla1:
	ld.l 0(r20),r16
	ld.l 0(r19),r17
	and r16,r24,r16
	and r17,r23,r17
	or r17,r16,r16
	st.l r16,0(r19)
	addu r22,r20,r20
	bla r31,r18,.bla1
	addu r22,r19,r19

	addu -1,r21,r21
	addu r25,r20,r20	// Increment srcPtr to next scanline
	btne r21,r0,.loop1
	br .L23
	nop
.L22:
	subs 16,r27,r0
	bte r16,r0,.rtol	// if (!ltor) do rtol copyrect
	bnc .rtol		// if (width <= 16) do rtol copyrect
	subu r20,r19,r16	// r16 = srcPtr - dstPtr
	and 7,r16,r0		// if (srcPtr not aligned with dstPtr)
	bnc .unaligned		//     do unaligned copyrect

//------------- Fast 2-at-a-time aligned ltor Copyrect ------------------
	adds -1,r0,r30		// Initialize bla decrement
	shl 2,r27,r29		//
	adds r29,r25,r25	// Adjust srcRowBytes to full scanline
	adds r29,r26,r26	// Adjust dstRowBytes to full scanline
	adds r19,r29,r29
	adds -4,r29,r29		// r29 = endPtr (last dst pixel in scanline)
.loop4:
	and 15,r19,r18
	subs 16,r18,r18		// r18 = 16 - (dstPtr&15) (pixels to quadword)
	mov r0,r31		// Draw first 1-4 pixels
.first4:
	fld.l r31(r20),f16	// Read starting 4 pixels
	fst.l f16,r31(r19)	// Write starting 4 pixels
	adds 4,r31,r31		// increment offset by 1 pixel	
	btne r31,r18,.first4	// until we reach r18

	andnot 15,r19,r31	// Round off dstPtr to correct quadword
	adds r20,r18,r22	// Adjust srcPtr to correct doubleword
	shr 2,r18,r18		// r18 = # pixels to quad dstPtr
	subs r27,r18,r18	// r18 = width - # pixels to quad dstPtr
	shr 2,r18,r18		// r18 = # 4-pixel-bunches to copy
	adds -2,r18,r18		// Adjust for bla loop

	fld.d 0(r22),f14
	bla r30,r18,.bla4	// Copies (r18+2)*4 pixels
	fld.d 8(r22)++,f18
.bla4:
	fmov.dd f14,f16
	fld.d 8(r22)++,f14
	fst.q f16,16(r31)++
	bla r30,r18,.bla4
	fld.d 8(r22)++,f18

	adds 4,r22,r22		// Bump src to 4 before next pixel
	fmov.dd f14,f16
	fst.q f16,16(r31)++
	adds 12,r31,r31		// Bump dst to 4 before next pixel
.last4:
	bte r31,r29,.sk4	// Do remaining pixels (to endPtr)
	fld.l 4(r22)++,f16
	br .last4
	fst.l f16,4(r31)++
.sk4:
	addu r25,r20,r20	// Increment srcPtr to next scanline
	addu -1,r21,r21
	addu r26,r19,r19	// Increment dstPtr to next scanline
	addu r26,r29,r29	// Increment endPtr to next scanline
	btne r21,r0,.loop4
	br .L23
	nop
//---------------------- Unaligned Copyrect ----------------------------
.unaligned:
	adds -2,r0,r30		// Initialize 2-at-a-time bla decrement
	shl 2,r27,r29		//
	adds r29,r25,r25	// Adjust srcRowBytes to full scanline
	adds r29,r26,r26	// Adjust dstRowBytes to full scanline
	adds -4,r29,r29		// Compute offset to end pixel
	and 7,r19,r16
	subu 8,r16,r16		// r16 = 8 - (dstPtr&7)
	and 7,r19,r0		// Test for dstPtr starting on odd pixel
	bc .loop3
	adds -1,r27,r27		// Started on odd pixel, so decrement width
.loop3:
	fld.l 0(r20),f16	// Read starting pixel
	adds -2,r27,r18		// Compute count for 2-at-a-time bla
	subs r19,r16,r31	// Round off dstPtr to correct doubleword
	fst.l f16,0(r19)	// Write starting pixel
	subs r20,r16,r22	// Round off srcPtr to correct doubleword
	bla r30,r18,.bla3
	adds 4,r22,r22		// Correct for different reading of srcPtr
.bla3:
	fld.l 4(r22)++,f17
	fld.l 4(r22)++,f16
	bla r30,r18,.bla3
	fst.d f16,8(r31)++

	fld.l r29(r20),f17	// Read ending pixel
	addu -1,r21,r21
	addu r25,r20,r20
	fst.l f17,r29(r19)	// Write ending pixel
	addu r26,r19,r19
	btne r21,r0,.loop3
	br .L23
	nop
//---------------------- Right-to-left Copyrect ----------------------------
.rtol:
	adds -1,r0,r31		// Initialize 1-at-a-time bla decrement
	subu r20,r22,r20	// Prepare srcPtr for autoinc fst
	subu r19,r22,r19	// Prepare dstPtr for autoinc fst
.loopr:
	adds -1,r27,r18		// Compute count for 1-at-a-time bla
	bla r31,r18,.blar
	nop
.blar:
	fld.l r22(r20)++,f16
	bla r31,r18,.blar
	fst.l f16,r22(r19)++

	addu -1,r21,r21
	addu r26,r19,r19
	addu r25,r20,r20
	btne r21,r0,.loopr
	br .L23
	nop
//---------------------- Constant Copyrect ----------------------------
.L31:
	adds -2,r0,r30		// Initialize 2-at-a-time bla decrement
	shl 2,r27,r29		//
	adds r29,r26,r26	// Adjust rowbytes to full scanline
	adds -4,r29,r29		// Compute offset to end pixel
	and 7,r19,r0		// Test for starting on odd pixel
	bc .loop0
	adds -1,r27,r27		// Started on odd pixel, so decrement width
.loop0:
	adds -2,r27,r18		// Compute count for 2-at-a-time bla
	fst.l f16,0(r19)	// Write starting pixel
	fst.l f16,r29(r19)	// Write ending pixel
	bc .skip0		// If width < 2, skip 2-at-a-time bla
	adds -4,r19,r31
	bla r30,r18,.bla0
	andnot 7,r31,r31	// Round off start to correct doubleword
.bla0:
	bla r30,r18,.bla0
	fst.d f16,8(r31)++
.skip0:
	addu -1,r21,r21
	addu r26,r19,r19
	btne r21,r0,.loop0
.L23:
	ld.l 4(fp),r1
	ld.l 0(fp),fp
	bri r1
	addu 16,sp,sp

