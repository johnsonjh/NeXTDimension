	.text
	nop	// FIR - 4 pager check value
gcc_compiled.:
.text
	.align 4
.globl _BMHighlight38
_BMHighlight38:
	adds -16,sp,sp
	st.l fp,8(sp)
	st.l r1,12(sp)
	adds 8,sp,fp
	ld.l 8(r16),r19
	ld.l 44(r16),r18
	ld.l 36(r16),r17
	ld.l 4(r16),r23
	adds -1,r0,r21		// r21 = 0xffffff00
	ixfr r21,f21		// f21 = r21 = 0xffffff00
	or l%-1431655936,r0,r20
	orh h%-1431655936,r20,r20
	or 255,r20,r20
	ixfr r20,f20		// f20 = r20 = 0xaaaaaaff
	bte r19,r0,.L15
	shl 2,r18,r22
	adds -1,r0,r31		// Initialize 1-at-a-time bla decrement
	adds -4,r17,r17		// Compensate for fst.l++
.loop1:
	adds -1,r23,r18		// Compute count for 1-at-a-time bla
	bla r31,r18,.blawhite
	nop
.blawhite:
	ld.l 4(r17),r16
	btne r16,r21,.other	// if (d != 0xffffffff) doother
.white:
	bla r31,r18,.blawhite
	fst.l f20,4(r17)++	// Write 0xaaaaaaff
	br .end1
	nop
.blagray:
	ld.l 4(r17),r16
	btne r16,r20,.other	// if (d != 0xaaaaaaff) doother
.gray:
	bla r31,r18,.blagray
	fst.l f21,4(r17)++	// Write 0xffffffff
	br .end1
	nop
.blaother:
	ld.l 4(r17),r16
.other:
	or  255,r16,r16		// r16 = d |= 0xff
	bte r16,r21,.white	// if (d == 0xffffffff) dowhite
	bte r16,r20,.gray	// if (d != 0xaaaaaaff) dogray
	bla r31,r18,.blaother
	addu 4,r17,r17
.end1:
	addu -1,r19,r19
	addu r22,r17,r17	// Increment dest ptr to next scanline
	btne r19,r0,.loop1
.L15:
	ld.l 4(fp),r1
	ld.l 0(fp),fp
	bri r1
	addu 16,sp,sp
