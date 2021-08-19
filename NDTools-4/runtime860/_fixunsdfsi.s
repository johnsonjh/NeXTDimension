	.file	"_fixunsdfsi.c"
// ccom  -O -X22 -X74 -X80 -X83 -X247 -X254 -X266 -X278 -X325 -X350 -X383 -X422
//	 -X424 -X501 -X523 -X524 -X525

	.text
	.align	4
___fixunsdfsi:
//	    .bf
	ld.c	fsr,r28
	andnot	12,r28,r27
	orh	ha%.L10,r0,r30
	fld.d	l%.L10(r30),f16
	or	4,r27,r27
	st.c	r27,fsr
//	    .ef
	fsub.dd	f8,f16,f18
	fix.dd	f18,f18
	st.c	r28,fsr
	fxfr	f18,r16
	bri	r1
	 xorh	32768,r16,r16
	.align	4
	.data
.L5:
.L10:	.byte	0,0,0,0
	.byte	0,0,224,65

//_a	f8	local

	.text
	.data
//_target_flags	_target_flags	import
	.globl	___fixunsdfsi

	.text
