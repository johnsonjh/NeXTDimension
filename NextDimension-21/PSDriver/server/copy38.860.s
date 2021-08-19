	.text
	nop	// FIR - 4 pager check value
gcc_compiled.:
.text
	.align 4
.globl _Copy38
_Copy38:
	adds -16,sp,sp
	st.l fp,8(sp)
	st.l r1,12(sp)
	adds 8,sp,fp
	mov r16,r27
	mov r17,r25
	mov r18,r24
	mov r19,r23
	mov r20,r26
	bte r22,r0,.L2
	and 3,r24,r31		// Is srcptr longword aligned?
	btne r31,r0,.slow1
	adds -1,r0,r31		// Initialize 1-at-a-time bla decrement
.fast1:				// RGBA longword aligned
	adds -4,r24,r18		// Prepare srcPtr for autoinc fst
	adds -1,r27,r19		// Compute count for 1-at-a-time bla
	bla r31,r19,.bla1
	adds -4,r23,r20		// Prepare dstPtr for autoinc fst
.bla1:
	fld.l 4(r18)++,f16
	bla r31,r19,.bla1
	fst.l f16,4(r20)++

	addu r26,r24,r24
	addu r21,r23,r23
	addu -1,r25,r25
	btne r25,r0,.fast1
	br .L9
	nop
	
.slow1:				// RGBA unaligned
	mov r24,r18		// r18 = sp = srcbase
	mov r23,r20		// r20 = dp = dstbase
	mov r27,r19		// r19 = w = width
.L6:
	ld.b 0(r18),r16
	ld.b 1(r18),r17
	ld.b 2(r18),r28
	ld.b 3(r18),r29
	shl 24,r16,r16
	and 0xff,r17,r17
	shl 16,r17,r17
	or r17,r16,r16
	and 0xff,r28,r28
	shl 8,r28,r28
	or r28,r16,r16
	and 0xff,r29,r29
	or r29,r16,r16
	st.l r16,0(r20)
	addu 4,r20,r20
	addu 4,r18,r18
	addu -1,r19,r19
	btne r19,r0,.L6

	addu r26,r24,r24
	addu r21,r23,r23
	addu -1,r25,r25
	btne r25,r0,.slow1
	br .L9
	nop
.L2:
.L10:				// RGB case
	mov r24,r18		// r18 = sp = srcbase
	mov r23,r20		// r20 = dp = dstbase
	mov r27,r19		// r19 = w = width
.L13:
	ld.b 0(r18),r16
	ld.b 1(r18),r17
	ld.b 2(r18),r28
	shl 24,r16,r16
	and 0xff,r17,r17
	shl 16,r17,r17
	or r17,r16,r16
	and 0xff,r28,r28
	shl 8,r28,r28
	or r28,r16,r16
	or 255,r16,r16
	st.l r16,0(r20)
	addu 4,r20,r20
	addu 3,r18,r18
	addu -1,r19,r19
	btne r19,r0,.L13

	addu r26,r24,r24
	addu r21,r23,r23
	addu -1,r25,r25
	btne r25,r0,.L10
.L9:
	ld.l 4(fp),r1
	ld.l 0(fp),fp
	bri r1
	addu 16,sp,sp
