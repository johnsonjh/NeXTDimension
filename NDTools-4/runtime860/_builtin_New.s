	.file	"_builtin_New.c"
// ccom  -O -X22 -X74 -X80 -X83 -X247 -X254 -X266 -X278 -X325 -X350 -X383 -X422
//	 -X424 -X501 -X523 -X524 -X525

	.text
	.align	4
___builtin_vec_new:
	adds	-32,sp,sp
	st.l	r1,28(sp)
	st.l	r4,24(sp)
	st.l	r5,20(sp)
	st.l	r6,16(sp)
	st.l	r7,12(sp)
//	    .bf
	st.l	r8,8(sp)
	st.l	r9,4(sp)
	mov	r16,r4
	mov	r18,r5
	mov	r19,r6
	adds	1,r17,r7
	btne	0,r16,.L4
	ixfr	r5,f10
	ixfr	r7,f8
	fmlow.dd	f8,f10,f8
	call	___builtin_new
	 fxfr	f8,r16
	mov	r16,r4
	mov	r16,r9
	subs	0,r7,r0
	mov	r0,r8
	bnc	.L6
	br	.L7
	 nop
.L4:
	mov	r4,r9
	subs	0,r7,r0
	mov	r0,r8
	bnc	.L6
.L7:
	calli	r6
	 mov	r4,r16
	adds	r5,r4,r4
	adds	1,r8,r8
	subs	r8,r7,r0
	bc	.L7
.L6:
	mov	r9,r16
//	    .ef
	ld.l	4(sp),r9
	ld.l	8(sp),r8
	ld.l	12(sp),r7
	ld.l	16(sp),r6
	ld.l	20(sp),r5
	ld.l	24(sp),r4
	ld.l	28(sp),r1
	adds	32,sp,sp
	bri	r1
	 nop
	.align	4
	.data
//_i	r8	local
//_nelts	r7	local
//_rval	r9	local
.L11:

//_p	r4	local
//_maxindex	r17	local
//_size	r5	local
//_ctor	r6	local

	.text
	.align	4
___set_new_handler:
//	    .bf
	orh	ha%___new_handler,r0,r30
	ld.l	l%___new_handler(r30),r17
	btne	0,r16,.L37
	orh	h%_default_new_handler,r0,r16
	or	l%_default_new_handler,r16,r16
	orh	ha%___new_handler,r0,r30
	st.l	r16,l%___new_handler(r30)
	bri	r1
	 mov	r17,r16
.L37:
	orh	ha%___new_handler,r0,r30
	st.l	r16,l%___new_handler(r30)
//	    .ef
	bri	r1
	 mov	r17,r16
	.align	4
	.data
//_prev_handler	r17	local
.L40:

//_handler	r16	local

	.text
	.align	4
_set_new_handler:
	adds	-16,sp,sp
//	    .bf
//	    .ef
	st.l	r1,12(sp)
	ld.l	12(sp),r1
	br	___set_new_handler
	 adds	16,sp,sp
	.align	4
	.data
.L59:

//_handler	r16	local

	.text
	.align	4
_default_new_handler:
//	    .bf
	adds	-16,sp,sp
	st.l	r1,12(sp)
	or	2,r0,r16
	orh	h%.L79,r0,r17
	or	l%.L79,r17,r17
	call	_write
	 or	65,r0,r18
//	    .ef
	ld.l	12(sp),r1
	adds	16,sp,sp
	br	__exit
	 adds	-1,r0,r16
	.align	4
	.data
//.L76	.L79	static
.L71:
.L79:	.byte	100,101,102,97
	.byte	117,108,116,95
	.byte	110,101,119,95
	.byte	104,97,110,100
	.byte	108,101,114,58
	.byte	32,111,117,116
	.byte	32,111,102,32
	.byte	109,101,109,111
	.byte	114,121,46,46
	.byte	46,32,97,97
	.byte	97,105,105,105
	.byte	105,105,105,101
	.byte	101,101,101,101
	.byte	101,101,101,101
	.byte	101,101,101,101
	.byte	101,33,10,0

	.text
	.data
//_target_flags	_target_flags	import
.L84:
___new_handler:	.long	_default_new_handler
	.globl	___new_handler
	.globl	_set_new_handler
	.globl	___set_new_handler
	.globl	___builtin_vec_new

	.text
