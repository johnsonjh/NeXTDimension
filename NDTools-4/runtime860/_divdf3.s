	.file	"_divdf3.c"
// ccom  -O -X22 -X74 -X80 -X83 -X247 -X254 -X266 -X278 -X325 -X350 -X383 -X422
//	 -X424 -X501 -X523 -X524 -X525

	.text
	.align	4
___divdf3:
//	    .bf
	fmov.dd	f8,f12
	frcp.dd	f10,f16
	fmul.dd	f10,f16,f8
	orh	16384,r0,r28
	ixfr	r28,f19
//	    .ef
	fmov.ss	f0,f18
	fsub.dd	f18,f8,f8
	fmul.dd	f16,f8,f16
	fmul.dd	f10,f16,f8
	fsub.dd	f18,f8,f8
	fmul.dd	f16,f8,f16
	fmul.dd	f10,f16,f8
	fsub.dd	f18,f8,f8
	fmul.dd	f12,f16,f16
	bri	r1
	 fmul.dd	f8,f16,f8
	.align	4
	.data
.L5:

//_a	f8	local
//_b	f10	local

	.text
	.data
//_target_flags	_target_flags	import
	.globl	___divdf3

	.text
