
//
//                       Integer division routines
//
//Converts ints to doubles does a double divide, and then converts the result
//back to an int.  Note: things can be sped up when dividing by a constant
//by converting the const to floating and taking the reciprical in the compiler.
//I do this.

	     .data
	     .align  8

two52two31:
		.long   0x43300000
		.long   0x80000000  //I don't trust floating conversion
two:
		.long   0x40000000
		.long   0x0
onepluseps:
		.long   0x3ff00000
		.long   0x1000
two52:
		.long   0x43300000
		.long   0x0


	    .text


// Signed integer divide, r16=r16/r17
// uses f16-f25
___divsi3::
				    // If numerator is < 32 and denominator
                                    // is positive & nonzero, use repeated
                                    // subtraction.
    subu 	32,r16,r0	    
    bc.t	.L996
    or 		r0,r17,r0	    // but only if r17 != 0
    br		.L999
    nop
.L996:
    bnc.t       .L997		    // No loads between dummy store and here.....
    andh	0x8000,r17,r18	    // also set r18 to 0 if we need it
    fiadd.ss    f0,f0,f16
    frcp.ss     f16,f0		    // r17=0 -> trap, but assembler wont
                                    // let me say frcp.ss f0,f0
.L997:
    bnc         .L999		    // No loads between dummy store and here.....
.L998:
    subs	r16,r17,r16
    bnc.t       .L998		    // No loads between dummy store and here.....
    addu	1,r18,r18
    bri		r1
    or		r18,r0,r16

.L999:
    fst.q       f16,-16(sp)++
    fst.q       f20,-16(sp)++
    fst.q       f24,-16(sp)++       // save two extra registers to align the sp

// Convert denominator (r17) and numerator (r16) into doubles (f18,f16)
    orh	ha%two52two31,r0,r31
    fld.d	l%two52two31(r31),f20
    xorh        0x8000,r17,r17
    ixfr        r17,f18
    fmov.ss     f21,f19             // Make f18.f19 a valid number by loading
				    // the correct exponant
    xorh        0x8000,r16,r16      // Start the other argument
     fsub.dd     f18,f20,f18        // Now f18.f19 is correct (put here to fill gap)
    ixfr        r16,f16
    fmov.ss     f21,f17
//   fsub.dd     f16,f20,f16         // moved down a little further

// Now do the divide
    orh		ha%two,r0,r31
    fld.d	l%two(r31),f24
     fsub.dd     f16,f20,f16        // Final part of the conversion
    frcp.dd     f18,f20             // Make a guess at the reciprical of denom
    fmul.dd     f18,f20,f22         // guess*divisor
    fsub.dd     f24,f22,f22         // 2-guess*divisor
    fmul.dd     f20,f22,f20         // second guess is off by 2^-15
    fmul.dd     f18,f20,f22         // guess*divisor
    fsub.dd     f24,f22,f22         // 2-guess*divisor
    fmul.dd     f20,f22,f20         // third guess is off by 2^-29
    fmul.dd     f18,f20,f22         // guess*divisor
    fsub.dd     f24,f22,f22         // 2-guess*divisor
    fmul.dd     f20,f16,f20         // guess*numerator
    fmul.dd     f22,f20,f22         // fixup by error term

// Round result a little, then convert to integer
    orh		ha%onepluseps,r0,r31
    fld.d	l%onepluseps(r31),f24	// load value 1+2**-40
    fmul.dd     f22,f24,f22         // force quotient to be bigger than integer
   fld.q       0(sp),f24
    ftrunc.dd   f22,f22             // convert to integer
   fld.q       32(sp),f16
    fxfr        f22,r16             // move to an integer reg

    fld.q       16(sp),f20
   bri         r1
    adds        48,sp,sp





// Unsigned integer divide, r16=r16/r17
// uses f16-f25
//   (same as above, except the conversion is easier)
___udivsi3::
    bte         1,r17,.L1           // avoid potential explosions
    fst.q       f16,-16(sp)++
    fst.q       f20,-16(sp)++
    fst.q       f24,-16(sp)++       // save two extra registers to align the sp

// Convert denominator (r17) and numerator (r16) into doubles (f18,f16)
    orh		ha%two52,r0,r31
    fld.d	l%two52(r31),f20
    ixfr        r17,f18
    ixfr        r16,f16
    fmov.ss     f21,f19             // Make f18.f19 a valid number by loading
				    // the correct exponant
    fmov.ss     f21,f17
    fsub.dd     f18,f20,f18         // Now f18.f19 is correct
    fsub.dd     f16,f20,f16         // moved down a little further

// Now do the divide
    orh		ha%two,r0,r31
    fld.d	l%two(r31),f24
    frcp.dd     f18,f20             // Make a guess at the reciprical of denom
    fmul.dd     f18,f20,f22         // guess*divisor
    fsub.dd     f24,f22,f22         // 2-guess*divisor
    fmul.dd     f20,f22,f20         // second guess is off by 2^-15
    fmul.dd     f18,f20,f22         // guess*divisor
    fsub.dd     f24,f22,f22         // 2-guess*divisor
    fmul.dd     f20,f22,f20         // third guess is off by 2^-29
    fmul.dd     f18,f20,f22         // guess*divisor
    fsub.dd     f24,f22,f22         // 2-guess*divisor
    fmul.dd     f20,f16,f20         // guess*numerator
    fmul.dd     f22,f20,f22         // fixup by error term

// Round result a little, then convert to integer
    orh		ha%onepluseps,r0,r31
    fld.d	l%onepluseps(r31),f24	// load value 1+2**-40
    fmul.dd     f22,f24,f22         // force quotient to be bigger than integer
   fld.q       0(sp),f24
    ftrunc.dd   f22,f22             // convert to integer
   fld.q       32(sp),f16
    fxfr        f22,r16             // move to an integer reg

    fld.q       16(sp),f20
   bri         r1
    adds        48,sp,sp

// The ftrunc instruction explodes on any unsigned number >= 0x80000000.
// This could happen on a divide if the numerator >= 0x80000000 and the
// denominator=1.  Since division by 1 is easy, I make a special check for
// it at the start and avoid the problem
.L1:
    bri         r1
    nop

// Signed integer remainder, r16=r16%r17
// uses f16-f25
___modsi3::
    fst.q       f16,-16(sp)++
    fst.q       f20,-16(sp)++
    fst.q       f24,-16(sp)++       // save two extra registers to align the sp

// Convert denominator (r17) and numerator (r16) into doubles (f18,f16)
    orh		ha%two52two31,r0,r31
    fld.d	l%two52two31(r31),f20
    xorh        0x8000,r17,r17
    ixfr        r17,f18
    fmov.ss     f21,f19             // Make f18.f19 a valid number by loading
				    // the correct exponant
    xorh        0x8000,r16,r16      // Start the other argument
     fsub.dd     f18,f20,f18        // Now f18.f19 is correct (put here to fill gap)
    ixfr        r16,f16
    fmov.ss     f21,f17
//   fsub.dd     f16,f20,f16         // moved down a little further

// Now do the divide
    orh		ha%two,r0,r31
    fld.d	l%two(r31),f24
     fsub.dd     f16,f20,f16        // Final part of the conversion
    frcp.dd     f18,f20             // Make a guess at the reciprical of denom
    fmul.dd     f18,f20,f22         // guess*divisor
    fsub.dd     f24,f22,f22         // 2-guess*divisor
    fmul.dd     f20,f22,f20         // second guess is off by 2^-15
    fmul.dd     f18,f20,f22         // guess*divisor
    fsub.dd     f24,f22,f22         // 2-guess*divisor
    fmul.dd     f20,f22,f20         // third guess is off by 2^-29
    fmul.dd     f18,f20,f22         // guess*divisor
    fsub.dd     f24,f22,f22         // 2-guess*divisor
    fmul.dd     f20,f16,f20         // guess*numerator
    fmul.dd     f22,f20,f22         // fixup by error term

// Round result a little, then compute remainder
    xorh        0x8000,r17,r17      // Get the right value of the denominator
    xorh        0x8000,r16,r16      //  and the numerator
//It might be possible to remove the above to instructions, test this.
    ixfr        r17,f16
    orh		ha%onepluseps,r0,r31
    fld.d	l%onepluseps(r31),f24	// load value 1+2**-40
    fmul.dd     f22,f24,f22         // force quotient to be bigger than integer

    ftrunc.dd   f22,f22             // convert to integer
   fld.q       0(sp),f24
    fmlow.dd    f16,f22,f22         // integer mult, quotient*denominator
   fld.q       32(sp),f16
    fxfr        f22,r17             // move to an integer reg
   fld.q       16(sp),f20
    subs        r16,r17,r16         // rem=numerator-quotient*denominator

   bri         r1
    adds        48,sp,sp




// Unsigned integer remainder, r16=r16%r17
// uses f16-f25
//   (same as above, except the conversion is easier)
___umodsi3::
    bte         1,r17,.L2
    fst.q       f16,-16(sp)++
    fst.q       f20,-16(sp)++
    fst.q       f24,-16(sp)++       // save two extra registers to align the sp

// Convert denominator (r17) and numerator (r16) into doubles (f18,f16)
    orh		ha%two52,r0,r31
    fld.d	l%two52(r31),f20
    ixfr        r17,f18
    ixfr        r16,f16
    fmov.ss     f21,f19             // Make f18.f19 a valid number by loading
				    // the correct exponant
    fmov.ss     f21,f17
    fsub.dd     f18,f20,f18         // Now f18.f19 is correct
    fsub.dd     f16,f20,f16         // moved down a little further

// Now do the divide
    orh		ha%two,r0,r31
    fld.d	l%two(r31),f24
    frcp.dd     f18,f20             // Make a guess at the reciprical of denom
    fmul.dd     f18,f20,f22         // guess*divisor
    fsub.dd     f24,f22,f22         // 2-guess*divisor
    fmul.dd     f20,f22,f20         // second guess is off by 2^-15
    fmul.dd     f18,f20,f22         // guess*divisor
    fsub.dd     f24,f22,f22         // 2-guess*divisor
    fmul.dd     f20,f22,f20         // third guess is off by 2^-29
    fmul.dd     f18,f20,f22         // guess*divisor
    fsub.dd     f24,f22,f22         // 2-guess*divisor
    fmul.dd     f20,f16,f20         // guess*numerator
    fmul.dd     f22,f20,f22         // fixup by error term

// Round result a little, then convert to integer
    ixfr        r17,f16
    orh		ha%onepluseps,r0,r31
    fld.d	l%onepluseps(r31),f24	// load value 1+2**-40
    fmul.dd     f22,f24,f22         // force quotient to be bigger than integer

    ftrunc.dd   f22,f22             // convert to integer
    fmlow.dd    f16,f22,f22         // integer mult, quotient*denominator

    fld.q       0(sp),f24
    fld.q       32(sp),f16
   fxfr         f22,r17             // move to an integer reg
    fld.q       16(sp),f20
   subu         r16,r17,r16         // rem=numerator-quotient*denominator
   bri         r1
    adds        48,sp,sp

// The ftrunc instruction explodes on any unsigned number >= 0x80000000.
// This could happen on a modulus if the numerator >= 0x80000000 and the
// denominator=1.  Since modulus by 1 is easy, I make a special check for
// it at the start and avoid the problem
.L2:
   bri         r1
    mov         r0,r16

