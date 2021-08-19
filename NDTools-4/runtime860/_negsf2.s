	.file	"_negsf2.c"
// ccom  -O -X22 -X74 -X80 -X83 -X247 -X254 -X266 -X278 -X325 -X350 -X383 -X422
//	 -X424 -X501 -X523 -X524 -X525

	.text
	.align	4
___negsf2:
	adds	-16,sp,sp
//	    .bf
//	    .ef
	fld.l	16(sp),f18
	fsub.ss	f0,f18,f19
	fst.l	f19,12(sp)
	ld.l	12(sp),r16
	adds	16,sp,sp
	bri	r1
	 nop
	.align	4
	.data
//_intify	-4(sp)	local
.L5:

//_a	0(sp)	local

	.text
	.data
//_target_flags	_target_flags	import
	.globl	___negsf2

	.text
