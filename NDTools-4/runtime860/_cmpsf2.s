	.file	"_cmpsf2.c"
// ccom  -O -X22 -X74 -X80 -X83 -X247 -X254 -X266 -X278 -X325 -X350 -X383 -X422
//	 -X424 -X501 -X523 -X524 -X525

	.text
	.align	4
___cmpsf2:
//	    .bf
	fld.l	4(sp),f19
	fld.l	0(sp),f18
	pfgt.ss	f18,f19,f0
	bnc	.L4
	bri	r1
	 or	1,r0,r16
.L4:
	fld.l	4(sp),f19
	fld.l	0(sp),f18
	pfgt.ss	f19,f18,f0
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

//_a	0(sp)	local
//_b	4(sp)	local

	.text
	.data
//_target_flags	_target_flags	import
	.globl	___cmpsf2

	.text
