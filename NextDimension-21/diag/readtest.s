.text
	PSR_IM = 0x10
	EPSR_BE = 0x0080	// High half of EPSR
	
	END_ADDR_PORT = 0x4800FFFC
	START_ADDR_PORT = 0x4800FFF8
	TESTINFO_PORT = 0x4800FFF4
	BAD_DATAHIGH_PORT = 0x4800FFF0
	BAD_DATALOW_PORT = 0x4800FFEC
	BAD_ADDR_PORT =	0x4800FFE8
	
	TEST_BYTE = 1
	TEST_SHORT = 2
	TEST_WORD = 4
	TEST_LONG = 8
	
_start::
	// mask all interrupts
	ld.c	psr,r16
	andnot	PSR_IM,r16,r16
	st.c	r16,psr
	ld.c	fir,r0		// Init FIR per PRM 7.8, para 2
	
	// Switch to big-endian mode
	ld.c	epsr,r16
	orh	EPSR_BE,r16,r16
	st.c	r16,epsr
	nop
	
	// Flush the data cache
	call	_flush
	nop

	ld.c	dirbase,r16
	or	0x20,r16,r16	// Set ITI to nuke instruction and AT caches
	st.c	r16,dirbase

	// Load registers up:
	// 	r16 holds start addr
	//	r17 holds end addr
	
	ld.l	START_ADDR_PORT(r0),r16
	ld.l	END_ADDR_PORT(r0),r17
	
	// Decide which test we should run
	ld.l	TESTINFO_PORT(r0),r18
	and	TEST_BYTE,r18,r0
	nop
	bnc	test_byte
	and	TEST_SHORT,r18,r0
	nop
	bnc	test_short
	and	TEST_WORD,r18,r0
	nop
	bnc	test_word
	and	TEST_LONG,r18,r0
	nop
	bnc	test_long
	// We should never get here.  hah...
spin0:
	br spin0
	nop

test_byte:
	bte	r16,r17,done	// At end of address range?
	ld.b	0(r16),r19	// Fetch next byte
	xor	r16,r19,r20
	and	0xFF,r20,r20
	bc.t	test_byte	// Continue if equal
	    addu	1,r16,r16	// Bump pointer to next value
	st.l	r19,BAD_DATALOW_PORT(r0)
	subu	1,r16,r16
	st.l	r16,BAD_ADDR_PORT(r0)
	addu	1,r16,r16
spin_badbyte:
	ld.l	BAD_ADDR_PORT(r0),r20
	or	r20,r20,r0
	bnc	spin_badbyte
	br	test_byte
	nop
	
test_short:
	andnot	1,r16,r16
	andnot	1,r17,r17	// Bash into correct alignment
loop_short:
	bte	r16,r17,done	// At end of address range?
	ld.s	0(r16),r19	// Fetch next short
	xor	r16,r19,r20
	and	0xFFFF,r20,r20
	bc.t	loop_short	// Continue if equal
	    addu	2,r16,r16	// Bump pointer to next value
	st.l	r19,BAD_DATALOW_PORT(r0)
	subu	2,r16,r16
	st.l	r16,BAD_ADDR_PORT(r0)
	addu	2,r16,r16
spin_badshort:
	ld.l	BAD_ADDR_PORT(r0),r20
	or	r20,r20,r0
	bnc	spin_badshort
	br	loop_short
	nop

test_word:
	andnot	3,r16,r16
	andnot	3,r17,r17	// Bash into correct alignment
loop_word:
	bte	r16,r17,done	// At end of address range?
	ld.l	0(r16),r19	// Fetch next word
	xor	r16,r19,r20
	bc.t	loop_word	// Continue if equal
	    addu	4,r16,r16	// Bump pointer to next value
	st.l	r19,BAD_DATALOW_PORT(r0)
	subu	4,r16,r16
	st.l	r16,BAD_ADDR_PORT(r0)
	addu	4,r16,r16
spin_badword:
	ld.l	BAD_ADDR_PORT(r0),r20
	or	r20,r20,r0
	bnc	spin_badword
	br	loop_word
	nop


test_long:
	andnot	7,r16,r16
	andnot	7,r17,r17
loop_long:
	bte	r16,r17,done	// At end of address range?
	fld.d	0(r16),f8	// Fetch next 64 bit value
	addu	8,r16,r16	// Bump pointer to next value
	fxfr	f8,r19		// r19 should equal r16
	fxfr	f9,r20		// r20 should be the complement of r19
	xor	r16,r19,r0	// Low half OK?
	bnc	bad_long
	xor	r19,r20,r20	// XOR of number and complement should be 0xFFFFFFFF
	addu	1,r20,r20
	or	r20,r20,r0	// Should be zero....
	bc	loop_long	// Continue if equal
bad_long:
	st.l	r19,BAD_DATALOW_PORT(r0)
	st.l	r20,BAD_DATAHIGH_PORT(r0)
	subu	8,r16,r16
	st.l	r16,BAD_ADDR_PORT(r0)
	addu	8,r16,r16
spin_badlong:
	ld.l	BAD_ADDR_PORT(r0),r20
	or	r20,r20,r0
	bnc	spin_badlong
	br	loop_long
	nop
//
//	Completed our side of the test
//	Poke a zero in the test data port.  The host is waiting for this
//	
done:
	st.l	r0,TESTINFO_PORT(r0)
	br	done
	nop
		
   
//
//	The more or less standard Intel flush function, with B.0 step
//	bug workarounds and whatnot.
//

	.data
	.align 16
_padding:
	.blkb 512
_bad:
	.blkb 4096	// Scratch region for cache flush

	.text
_flush::
	ld.c	psr,r22
	nop
	andnot	PSR_IM,r22,r16
	st.c	r16,psr		//no interrupts for this operation

	ld.c	dirbase,r17

	adds	-1,r0,r19
	or	127,r0,r20
	or	l%_bad,r0,r16
	orh	h%_bad,r16,r16	//pick an unused virtual address
	adds	-32,r16,r16
	mov	r16,r21

	mov	r17,r18
	nop
	andnot	0xf00,r18,r18
	or	0x800,r18,r18
	st.c	r18,dirbase	//RC = 2, RB = 0

	ld.l	32(r16),r23
	ld.l	32(r16),r23	// CHIP BUG	B0 bug #10
	addu	r23,r0,r0	//clear the address pipe

	bla	r19,r20,nflp1
	nop
nflp1:
	bla	r19,r20,nflp1
	flush	32(r16)++

	ld.l	-512(r16),r0	//clear the address pipe

	or	0x100,r18,r18
	st.c	r18,dirbase	//RC = 2, RB = 1

	or	127,r0,r20
	mov	r21,r16
	adds	4096,r16,r16

	ld.l	32(r16),r23
	ld.l	32(r16),r23	// CHIP BUG	B0 bug #10
	addu	r23,r0,r0	//clear the address pipe

	bla	r19,r20,nflp2
	nop
nflp2:
	bla	r19,r20,nflp2
	flush	32(r16)++

	ld.l	-512(r16),r0	//clear the address pipe

	st.c	r17,dirbase
	st.c	r22,psr
	bri	r1
	nop
	
