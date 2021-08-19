	.file	"_lshrsi3.c"
// ccom  -O -X22 -X74 -X80 -X83 -X247 -X254 -X266 -X278 -X325 -X350 -X383 -X422
//	 -X424 -X501 -X523 -X524 -X525

	.text
	.align	4
___lshrsi3:
//	    .bf
//	    .ef
	bri	r1
	 shr	r17,r16,r16
	.align	4
	.data
.L5:

//_a	r16	local
//_b	r17	local

	.text
	.data
//_target_flags	_target_flags	import
	.globl	___lshrsi3

	.text
