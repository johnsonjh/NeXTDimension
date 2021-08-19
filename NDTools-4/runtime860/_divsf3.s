	.file	"_divsf3.s"
//
// SINGLE_PRECISION DIVIDE
//
//	The dividend is in f8.
//	The divisor is in f9.
//	The result is left in f8
//
	.text
	.align	4
	.globl	___divsf3
___divsf3:
	fmov.ss	f8,f19
	orh	16384,r0,r28		// Get 2.0 into f17
	ixfr	r28,f17

	frcp.ss	f9,f16			// Newton-Raphson approximate the reciprocal
	fmul.ss	f9,f16,f8
	fsub.ss	f17,f8,f8
	fmul.ss	f16,f8,f16
	fmul.ss	f9,f16,f8
	fsub.ss	f17,f8,f8
	fmul.ss	f19,f16,f17
	bri	r1
	    fmul.ss	f8,f17,f8	// Mult dividend by reciprocal

