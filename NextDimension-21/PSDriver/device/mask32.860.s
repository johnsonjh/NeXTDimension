	.text
	nop	// FIR - 4 pager check value
gcc_compiled.:
.text
	.align 4
.globl _ConstantMasksMark32
_ConstantMasksMark32:
	adds -16,sp,sp
	st.l fp,8(sp)
	st.l r1,12(sp)
	adds 8,sp,fp
	st.l r4,0(sp)
	st.l r5,4(sp)
	mov r16,r24
	mov r17,r25
	bte r25,r0,.L2
	mov r18,r28
	fld.l 20(r28),f16
	orh ha%_framebytewidth,r0,r31
	ld.l l%_framebytewidth(r31),r26
	ld.l 0(r28),r29		// r29 = args->markInfo
	or l%.L36,r0,r30	// setup for switch statement later
	orh h%.L36,r30,r30
	ld.l 4(r29),r28		// r28 = args->markInfo->offset.y
	ld.l 0(r29),r29		// r29 = args->markInfo->offset.x
	orh ha%_framebase,r0,r31
	ld.l l%_framebase(r31),r4 // /NextApps/Term = _framebase
	shl 2,r29,r29		// multiply by 4 for pointer add
	ixfr r26,f8
.L34:
	ld.l 8(r24),r21		// r21 = masks->mask
	ld.l 4(r24),r17		// r17 = masks->dc.y
	ld.s 0(r21),r27		// r27 = mask->width
	ld.l 4(r21),r22		// r22 = sourceunit
	bte r27,r0,.L4		// if (mask->width == 0) continue
	addu r28,r17,r31	// r31 = y = masks->dc.y + offset.y
	ixfr r31,f10
	ld.l 0(r24),r17		// r17 = masks->dc.x
	fmlow.dd f8,f10,f10	// f10 = y * fbw
	shl 2,r17,r17
	ld.s 8(r21),r16		// r16 = mask->unused (implies byte source)
	fxfr f10,r18		// r18 = y * fbw
	and 32767,r16,r16
	ld.s 2(r21),r21		// r21 = height
	bc .dolong		// if (mask->unused) {
	or l%.getbyte,r0,r31	//     r5  = .getbyte
	orh h%.getbyte,r31,r5
	addu 7,r27,r27		//     r27 = mask->width + 7
	br .next
	shra 3,r27,r27		//     r27 = width = (mask->width+7)>>3;
.dolong:			// } else {
	or l%.getlong,r0,r31	//     r5  = .getlong
	orh h%.getlong,r31,r5
	addu 31,r27,r27		//     r27 = mask->width + 31
	shra 5,r27,r27		//     r27 = width = (mask->width+31)>>5;
.next:				
	addu r4,r18,r18		//     y * fbw + framebase 
	addu r17,r18,r18	//     + masks->dc.x
	addu r29,r18,r23	//     + args->markInfo->offset.x = r23
.L6:
	mov r27,r19
	bri r5			// Jump to either .getbyte or .getlong
	adds -16,r23,r31

.getbyte:
	ld.b 0(r22),r18		// su = *sourceunit++ (byte)
	addu 1,r22,r22
	mov r31,r17
	adds 32,r31,r31		// Move dPtr along by 8 longs
	br .L140
	shl 24,r18,r18		

.getlong:
	ld.l 0(r22),r18		// su = *sourceunit++
	mov r31,r17
	adds 128,r31,r31	// Move dPtr along by 32 longs
	br .L140
	addu 4,r22,r22
.L14:
	shl 4,r18,r18
.L140:
	bte r18,r0,.L35		// while (su != 0) {
	shr 28,r18,r16
	shl 4,r16,r16
	adds r30,r16,r16
	bri r16			// switch(su >> 28) {
	adds 16,r17,r17
.L36:
	br .L14
	nop
	nop
	nop
.L16:
	br .L14
	fst.l f16,12(r17)
	nop
	nop
.L17:
	br .L14
	fst.l f16,8(r17)
	nop
	nop
.L18:
	fst.l f16,8(r17)
	br .L14
	fst.l f16,12(r17)
	nop
.L40:
	br .L14
	fst.l f16,4(r17)
	nop
	nop
.L20:
	fst.l f16,4(r17)
	br .L14
	fst.l f16,12(r17)
	nop
.L41:
	fst.l f16,4(r17)
	br .L14
	fst.l f16,8(r17)
	nop
.L42:
	fst.l f16,4(r17)
	fst.l f16,8(r17)
	br .L14
	fst.l f16,12(r17)
.L23:
	br .L14
	fst.l f16,0(r17)
	nop
	nop
.L24:
	fst.l f16,0(r17)
	br .L14
	fst.l f16,12(r17)
	nop
.L25:
	fst.l f16,0(r17)
	br .L14
	fst.l f16,8(r17)
	nop
.L26:
	fst.l f16,0(r17)
	fst.l f16,8(r17)
	br .L14
	fst.l f16,12(r17)
.L27:
	fst.l f16,0(r17)
	br .L14
	fst.l f16,4(r17)
	nop
.L28:
	fst.l f16,0(r17)
	fst.l f16,4(r17)
	br .L14
	fst.l f16,12(r17)
.L29:
	fst.l f16,0(r17)
	fst.l f16,4(r17)
	br .L14
	fst.l f16,8(r17)
.L30:
	fst.l f16,0(r17)
	fst.l f16,4(r17)
	fst.l f16,8(r17)
	br .L14
	fst.l f16,12(r17)
.L35:
	addu -1,r19,r19
	bte r19,r0,.exit	// Get out of the loop
	bri r5			// Jump to either .getbyte or .getlong
	nop
.exit:	
	addu -1,r21,r21
	addu r26,r23,r23
	btne r21,r0,.L6
.L4:
	addu 12,r24,r24
	addu -1,r25,r25
	btne r25,r0,.L34
.L2:
	ld.l -8(fp),r4
	ld.l -4(fp),r5
	ld.l 4(fp),r1
	ld.l 0(fp),fp
	bri r1
	addu 16,sp,sp
