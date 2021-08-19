	.file	"_mulsf3.c"
// ccom  -O -X22 -X74 -X80 -X83 -X247 -X254 -X266 -X278 -X325 -X350 -X383 -X422
//	 -X424 -X501 -X523 -X524 -X525

	.text
	.align	4
___mulsf3:
	adds	-16,sp,sp
//	    .bf
	fld.l	20(sp),f19
//	    .ef
	fld.l	16(sp),f18
	fmul.ss	f19,f18,f19
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
//_b	4(sp)	local

	.text
	.data
//_target_flags	_target_flags	import
	.globl	___mulsf3

	.text
