	.file	"_floatsidf.c"
// ccom  -O -X22 -X74 -X80 -X83 -X247 -X254 -X266 -X278 -X325 -X350 -X383 -X422
//	 -X424 -X501 -X523 -X524 -X525

	.text
	.align	4
___floatsidf:
	orh	ha%.L12,r0,r30
	fld.d	l%.L12(r30),f8
//	    .bf
	xorh	32768,r16,r27
	ixfr	r27,f18
	fmov.ss	f9,f19
//	    .ef
	bri	r1
	 fsub.dd	f18,f8,f8
	.align	4
	.data
.L5:
.L12:	.byte	0,0,0,128
	.byte	0,0,48,67

//_a	r16	local

	.text
	.data
//_target_flags	_target_flags	import
	.globl	___floatsidf

	.text
