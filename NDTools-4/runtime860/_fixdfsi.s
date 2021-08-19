	.file	"_fixdfsi.c"
// ccom  -O -X22 -X74 -X80 -X83 -X247 -X254 -X266 -X278 -X325 -X350 -X383 -X422
//	 -X424 -X501 -X523 -X524 -X525

	.text
	.align	4
___fixdfsi:
//	    .bf
	ftrunc.dd	f8,f18
//	    .ef
	bri	r1
	 fxfr	f18,r16
	.align	4
	.data
.L5:

//_a	f8	local

	.text
	.data
//_target_flags	_target_flags	import
	.globl	___fixdfsi

	.text
