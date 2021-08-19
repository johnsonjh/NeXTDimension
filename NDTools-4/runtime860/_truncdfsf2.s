	.file	"_truncdfsf2.c"
// ccom  -O -X22 -X74 -X80 -X83 -X247 -X254 -X266 -X278 -X325 -X350 -X383 -X422
//	 -X424 -X501 -X523 -X524 -X525

	.text
	.align	4
___truncdfsf2:
//	    .bf
	fmov.ds	f8,f19
//	    .ef
	adds	-16,sp,sp
	fst.l	f19,12(sp)
	ld.l	12(sp),r16
	adds	16,sp,sp
	bri	r1
	 nop
	.align	4
	.data
//_intify	-4(sp)	local
.L5:

//_a	f8	local

	.text
	.data
//_target_flags	_target_flags	import
	.globl	___truncdfsf2

	.text
