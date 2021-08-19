	.file	"_builtin_del.c"
// ccom  -O -X22 -X74 -X80 -X83 -X247 -X254 -X266 -X278 -X325 -X350 -X383 -X422
//	 -X424 -X501 -X523 -X524 -X525

	.text
	.align	4
___builtin_delete:
	adds	-16,sp,sp
	st.l	r1,12(sp)
//	    .bf
	bte	0,r16,.L4
	ld.l	12(sp),r1
	br	_free
	 adds	16,sp,sp
.L4:
//	    .ef
	ld.l	12(sp),r1
	adds	16,sp,sp
	bri	r1
	 nop
	.align	4
	.data
.L6:

//_ptr	r16	local

	.text
	.align	4
___builtin_vec_delete:
	adds	-48,sp,sp
	st.l	r1,44(sp)
	st.l	r4,40(sp)
	st.l	r5,36(sp)
//	    .bf
	mov	r18,r5
	st.l	r6,32(sp)
	mov	r19,r6
	st.l	r7,28(sp)
	mov	r20,r7
	ixfr	r18,f16
	st.l	r8,24(sp)
	st.l	r9,20(sp)
	adds	1,r17,r9
	ixfr	r9,f18
	mov	r16,r4
	st.l	r10,16(sp)
	fmlow.dd	f18,f16,f18
	st.l	r11,12(sp)
	mov	r16,r11
	fxfr	f18,r16
	mov	r21,r8
	adds	r16,r4,r4
	subs	0,r9,r0
	mov	r0,r10
	bnc	.L24
	subs	r4,r5,r16
.L1000025:
	mov	r8,r17
	calli	r6
	 mov	r16,r4
	adds	1,r10,r10
	subs	r10,r9,r0
	bc.t	.L1000025
	 subs	r4,r5,r16
.L24:
	bte	0,r7,.L28
	call	_free
	 mov	r11,r16
	br	.L22
	 nop
.L28:
.L22:
//	    .ef
	ld.l	12(sp),r11
	ld.l	16(sp),r10
	ld.l	20(sp),r9
	ld.l	24(sp),r8
	ld.l	28(sp),r7
	ld.l	32(sp),r6
	ld.l	36(sp),r5
	ld.l	40(sp),r4
	ld.l	44(sp),r1
	adds	48,sp,sp
	bri	r1
	 nop
	.align	4
	.data
//_i	r10	local
//_nelts	r9	local
//_p	r11	local
.L30:

//_ptr	r4	local
//_maxindex	r17	local
//_size	r5	local
//_dtor	r6	local
//_auto_delete_vec	r7	local
//_auto_delete	r8	local

	.text
	.data
//_target_flags	_target_flags	import
	.globl	___builtin_vec_delete
	.globl	___builtin_delete

	.text
