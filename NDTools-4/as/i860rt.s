//
//         INTEL CORPORATION PROPRIETARY INFORMATION
//
//    This software is supplied under the terms of a license
//    agreement or nondisclosure agreement with Intel Corpo-
//    ration and may not be copied or disclosed except in
//    accordance with the terms of that agreement.
//

//
//                       Integer division routines
//
// Converts ints to doubles, does a double divide, and then converts the result
// back to an int.

	.data
	.align	8

.two52two31:
	.long	0x80000000
	.long	0x43300000
.two:
	.long	0x00000000
	.long	0x40000000
.onepluseps:
	.long	0x00001000
	.long	0x3ff00000
.two52:
	.long	0x00000000
	.long	0x43300000

	.text


// Signed integer divide, r16=r16/r17
// uses f16-f25
_DIVI::

// Convert denominator (r17) and numerator (r16) into doubles (f18,f16)
	fld.d	.two52two31,f20
	xorh	0x8000,r17,r18
	ixfr	r18,f18
	fmov.ss	f21,f19
	xorh	0x8000,r16,r18
	fsub.dd	f18,f20,f18
	ixfr	r18,f16
	fmov.ss	f21,f17
	fld.d	.two,f24
	fsub.dd	f16,f20,f16

// Do floating-point divide
	frcp.dd	f18,f20			// first guess has 2**-8 error
	fmul.dd	f18,f20,f22		// guess*divisor
	fsub.dd	f24,f22,f22		// 2-guess*divisor
	fmul.dd	f20,f22,f20		// second guess has 2**-15 error
	fmul.dd	f18,f20,f22		// guess*divisor
	fsub.dd	f24,f22,f22		// 2-guess*divisor
	fmul.dd	f20,f22,f20		// third guess has 2**-29 error
	fmul.dd	f18,f20,f22		// guess*divisor
	fsub.dd	f24,f22,f22		// 2-guess*divisor
	fmul.dd	f20,f16,f20		// guess*dividend
	fmul.dd	f22,f20,f22		// result = third guess*dividend

// Convert Quotient to integer
	fld.d	.onepluseps,f24		// load value 1+2**-40
	fmul.dd	f22,f24,f22		// force quotient to be bigger than int
	ftrunc.dd	f22,f22		// convert to integer
	bri	r1			// return
	fxfr	f22,r16			// transfer quotient


// Unsigned integer divide, r16=r16/r17
// uses f16-f25
// (same as above, except the conversion is easier)
_UDIVI::
	bte	1,r17,.L1		// avoid potential explosions

// Convert denominator (r17) and numerator (r16) into doubles (f18,f16)
	fld.d	.two52,f20
	ixfr	r17,f18
	fmov.ss	f21,f19
	fsub.dd	f18,f20,f18
	ixfr	r16,f16
	fmov.ss	f21,f17
	fld.d	.two,f24
	fsub.dd	f16,f20,f16

// Do floating-point divide
	frcp.dd	f18,f20			// first guess has 2**-8 error
	fmul.dd	f18,f20,f22		// guess*divisor
	fsub.dd	f24,f22,f22		// 2-guess*divisor
	fmul.dd	f20,f22,f20		// second guess has 2**-15 error
	fmul.dd	f18,f20,f22		// guess*divisor
	fsub.dd	f24,f22,f22		// 2-guess*divisor
	fmul.dd	f20,f22,f20		// third guess has 2**-29 error
	fmul.dd	f18,f20,f22		// guess*divisor
	fsub.dd	f24,f22,f22		// 2-guess*divisor
	fmul.dd	f20,f16,f20		// guess*dividend
	fmul.dd	f22,f20,f22		// result = third guess*dividend

// Convert Quotient to integer
	fld.d	.onepluseps,f24		// load value 1+2**-40
	fmul.dd	f22,f24,f22		// force quotient to be bigger than int
	ftrunc.dd	f22,f22		// convert to integer
	bri	r1			// return
	fxfr	f22,r16			// transfer quotient

// The ftrunc instruction explodes on any unsigned number >= 0x80000000.
// This could happen on a divide if the numerator >= 0x80000000 and the
// denominator=1.  Since division by 1 is easy, a special check for it
// is made at the start to avoid the problem
.L1:
	bri	r1			// return
	nop


// Signed integer remainder, r16=r16%r17
// uses f16-f25
_MODI::

// Convert denominator (r17) and numerator (r16) into doubles (f18,f16)
	fld.d	.two52two31,f20
	xorh	0x8000,r17,r18
	ixfr	r18,f18
	fmov.ss	f21,f19
	xorh	0x8000,r16,r18
	fsub.dd	f18,f20,f18
	ixfr	r18,f16
	fmov.ss	f21,f17
	fld.d	.two,f24
	fsub.dd	f16,f20,f16

// Do floating-point divide
	frcp.dd	f18,f20			// first guess has 2**-8 error
	fmul.dd	f18,f20,f22		// guess*divisor
	fsub.dd	f24,f22,f22		// 2-guess*divisor
	fmul.dd	f20,f22,f20		// second guess has 2**-15 error
	fmul.dd	f18,f20,f22		// guess*divisor
	fsub.dd	f24,f22,f22		// 2-guess*divisor
	fmul.dd	f20,f22,f20		// third guess has 2**-29 error
	fmul.dd	f18,f20,f22		// guess*divisor
	fsub.dd	f24,f22,f22		// 2-guess*divisor
	fmul.dd	f20,f16,f20		// guess*dividend
	fmul.dd	f22,f20,f22		// result = third guess*dividend

// Compute remainder
	fld.d	.onepluseps,f24		// load value 1+2**-40
	fmul.dd	f22,f24,f22		// force quotient to be bigger than int
	ixfr	r17,f16			// get denominator for rem computation
	ftrunc.dd	f22,f22		// convert to integer
	fmlow.dd	f16,f22,f22	// quotient*denominator
	fxfr	f22,r17			// move to an integer reg
	bri	r1			// return
	subs	r16,r17,r16		// rem=numerator-quotient*denominator


// Unsigned integer remainder, r16=r16%r17
// uses f16-f25
// (same as above, except the conversion is easier)
_UMODI::
	bte	1,r17,.L2		// avoid potential explosions

// Convert denominator (r17) and numerator (r16) into doubles (f18,f16)
	fld.d	.two52,f20
	ixfr	r17,f18
	fmov.ss	f21,f19
	fsub.dd	f18,f20,f18
	ixfr	r16,f16
	fmov.ss	f21,f17
	fld.d	.two,f24
	fsub.dd	f16,f20,f16

// Do floating-point divide
	frcp.dd	f18,f20			// first guess has 2**-8 error
	fmul.dd	f18,f20,f22		// guess*divisor
	fsub.dd	f24,f22,f22		// 2-guess*divisor
	fmul.dd	f20,f22,f20		// second guess has 2**-15 error
	fmul.dd	f18,f20,f22		// guess*divisor
	fsub.dd	f24,f22,f22		// 2-guess*divisor
	fmul.dd	f20,f22,f20		// third guess has 2**-29 error
	fmul.dd	f18,f20,f22		// guess*divisor
	fsub.dd	f24,f22,f22		// 2-guess*divisor
	fmul.dd	f20,f16,f20		// guess*dividend
	fmul.dd	f22,f20,f22		// result = third guess*dividend

// Compute remainder
	fld.d	.onepluseps,f24		// load value 1+2**-40
	fmul.dd	f22,f24,f22		// force quotient to be bigger than int
	ixfr	r17,f16			// get denominator for rem computation
	ftrunc.dd	f22,f22		// convert to integer
	fmlow.dd	f16,f22,f22	// quotient*denominator
	fxfr	f22,r17			// move to an integer reg
	bri	r1			// return
	subu	r16,r17,r16		// rem=numerator-quotient*denominator

// The ftrunc instruction explodes on any unsigned number >= 0x80000000.
// This could happen on a modulus if the numerator >= 0x80000000 and the
// denominator=1.  Since modulus by 1 is easy, a special check for it
// is made at the start to avoid the problem
.L2:
	bri	r1			// return
	mov	r0,r16			// rem=0
	
