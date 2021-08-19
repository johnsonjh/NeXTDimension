	.file	"_varargs.c"
// ccom  -O -X22 -X74 -X80 -X83 -X247 -X254 -X266 -X278 -X325 -X350 -X383 -X422
//	 -X424 -X501 -X523 -X524 -X525

	.text
	.align	4
___builtin_saveregs:
	adds	-16,sp,sp
//	    .bf
//	    .ef
	st.l	r1,12(sp)
	ld.l	12(sp),r1
	br	_abort
	 adds	16,sp,sp
	.align	4
	.data
.L4:

	.text
	.data
//_target_flags	_target_flags	import
	.globl	___builtin_saveregs

	.text
