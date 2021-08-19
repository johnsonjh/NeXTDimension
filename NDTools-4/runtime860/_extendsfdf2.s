	.file	"_extendsfdf2.c"
// ccom  -O -X22 -X74 -X80 -X83 -X247 -X254 -X266 -X278 -X325 -X350 -X383 -X422
//	 -X424 -X501 -X523 -X524 -X525

	.text
	.align	4
___extendsfdf2:
//	    .bf
	fld.l	0(sp),f20
//	    .ef
	bri	r1
	 fmov.sd	f20,f8
	.align	4
	.data
.L5:

//_a	0(sp)	local

	.text
	.data
//_target_flags	_target_flags	import
	.globl	___extendsfdf2

	.text
