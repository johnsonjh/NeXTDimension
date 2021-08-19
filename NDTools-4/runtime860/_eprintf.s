	.file	"_eprintf.c"
// ccom  -O -X22 -X74 -X80 -X83 -X247 -X254 -X266 -X278 -X325 -X350 -X383 -X422
//	 -X424 -X501 -X523 -X524 -X525

	.text
	.align	4
___eprintf:
	adds	-16,sp,sp
	st.l	r1,12(sp)
//	    .bf
//	    .ef
	mov	r16,r19
	ld.l	12(sp),r1
	adds	16,sp,sp
	orh	h%__iob+32,r0,r16
	mov	r18,r21
	mov	r17,r20
	mov	r20,r18
	mov	r19,r17
	mov	r21,r19
	br	_fprintf
	 or	l%__iob+32,r16,r16
	.align	4
	.data
.L4:

//_string	r19	local
//_line	r20	local
//_filename	r21	local

	.text
	.data
//_target_flags	_target_flags	import
//__iob	__iob	import
	.globl	___eprintf

	.text
