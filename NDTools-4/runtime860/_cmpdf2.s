	.file	"_cmpdf2.c"
// ccom  -O -X22 -X74 -X80 -X83 -X247 -X254 -X266 -X278 -X325 -X350 -X383 -X422
//	 -X424 -X501 -X523 -X524 -X525

	.text
	.align	4
___cmpdf2:
//	    .bf
	pfgt.dd	f8,f10,f0
	bnc	.L4
	bri	r1
	 or	1,r0,r16
.L4:
	pfgt.dd	f10,f8,f0
	bnc	.L7
	bri	r1
	 adds	-1,r0,r16
.L7:
//	    .ef
	bri	r1
	 mov	r0,r16
	.align	4
	.data
.L12:

//_a	f8	local
//_b	f10	local

	.text
	.data
//_target_flags	_target_flags	import
	.globl	___cmpdf2

	.text
