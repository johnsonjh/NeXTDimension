	.file	"_builtin_new.c"
// ccom  -O -X22 -X74 -X80 -X83 -X247 -X254 -X266 -X278 -X325 -X350 -X383 -X422
//	 -X424 -X501 -X523 -X524 -X525

	.text
	.align	4
___builtin_new:
	adds	-16,sp,sp
	st.l	r1,12(sp)
//	    .bf
	call	_malloc
	 st.l	r4,8(sp)
	mov	r16,r4
	btne	0,r16,.L4
	orh	ha%___new_handler,r0,r30
	ld.l	l%___new_handler(r30),r30
	calli	r30
	 nop
	mov	r4,r16
	ld.l	8(sp),r4
	ld.l	12(sp),r1
	adds	16,sp,sp
	bri	r1
	 nop
.L4:
	mov	r4,r16
//	    .ef
	ld.l	8(sp),r4
	ld.l	12(sp),r1
	adds	16,sp,sp
	bri	r1
	 nop
	.align	4
	.data
//_p	r4	local
.L7:

//_sz	r16	local

	.text
	.data
//_target_flags	_target_flags	import
//___new_handler	___new_handler	import
	.globl	___builtin_new

	.text
