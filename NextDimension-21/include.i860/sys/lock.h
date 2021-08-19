/* 
 * Mach Operating System
 * Copyright (c) 1987 Carnegie-Mellon University
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 */
/*
 *	File:	h/lock.h
 *	Author:	Avadis Tevanian, Jr., Michael Wayne Young
 *
 *	Copyright (C) 1985, Avadis Tevanian, Jr., Michael Wayne Young
 *
 *	Locking primitives definitions
 *
 * HISTORY
 * $Log:	lock.h,v $
 * Revision 2.1  88/11/25  13:05:58  rvb
 * 2.1
 * 
 * Revision 2.3  88/08/24  02:33:07  mwyoung
 * 	Adjusted include file references.
 * 	[88/08/17  02:15:53  mwyoung]
 * 
 * Revision 2.2  88/07/20  16:49:35  rpd
 * Allow for sanity-checking of simple locking on uniprocessors,
 * controlled by new option MACH_LDEBUG.  Define composite
 * MACH_SLOCKS, which is true iff simple locking calls expand
 * to code.  It can be used to #if-out declarations, etc, that
 * are only used when simple locking calls are real.
 * 
 *  3-Nov-87  David Black (dlb) at Carnegie-Mellon University
 *	Use optimized lock structure for multimax also.
 *
 * 27-Oct-87  Robert Baron (rvb) at Carnegie-Mellon University
 *	Use optimized lock "structure" for balance now that locks are
 *	done inline.
 *
 * 26-Jan-87  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Invert logic of no_sleep to can_sleep.
 *
 * 29-Dec-86  David Golub (dbg) at Carnegie-Mellon University
 *	Removed BUSYP, BUSYV, adawi, mpinc, mpdec.  Defined the
 *	interlock field of the lock structure to be a simple-lock.
 *
 *  9-Nov-86  Michael Young (mwyoung) at Carnegie-Mellon University
 *	Added "unsigned" to fields in vax case, for lint.
 *
 * 21-Oct-86  Michael Young (mwyoung) at Carnegie-Mellon University
 *	Added fields for sleep/recursive locks.
 *
 *  7-Oct-86  David L. Black (dlb) at Carnegie-Mellon University
 *	Merged Multimax changes.
 *
 * 26-Sep-86  Michael Young (mwyoung) at Carnegie-Mellon University
 *	Removed reference to "caddr_t" from BUSYV/P.  I really
 *	wish we could get rid of these things entirely.
 *
 * 24-Sep-86  Michael Young (mwyoung) at Carnegie-Mellon University
 *	Changed to directly import boolean declaration.
 *
 *  1-Aug-86  David Golub (dbg) at Carnegie-Mellon University
 *	Added simple_lock_try, sleep locks, recursive locking.
 *
 * 11-Jun-86  Bill Bolosky (bolosky) at Carnegie-Mellon University
 *	Removed ';' from definitions of locking macros (defined only
 *	when NCPU < 2). so as to make things compile.
 *
 * 28-Feb-86  Bill Bolosky (bolosky) at Carnegie-Mellon University
 *	Defined adawi to be add when not on a vax.
 *
 * 07-Nov-85  Michael Wayne Young (mwyoung) at Carnegie-Mellon University
 *	Overhauled from previous version.
 */

#ifndef	_LOCK_
#define	_LOCK_

#ifdef	KERNEL
#include <cpus.h>
#include <mach_ldebug.h>
#else	/* KERNEL */
#include <sys/features.h>
#endif	/* KERNEL */

#include <sys/boolean.h>

/*
 *	A simple spin lock.
 */

struct slock {
	int		lock_data;	/* in general 1 bit is sufficient */
};

typedef struct slock	simple_lock_data_t;
typedef struct slock	*simple_lock_t;

/*
 *	The general lock structure.  Provides for multiple readers,
 *	upgrading from read to write, and sleeping until the lock
 *	can be gained.
 */

struct lock {
#if	defined(vax)
	/*
	 *	Efficient VAX implementation -- see field description below.
	 */
	unsigned int	read_count:16,
			want_upgrade:1,
			want_write:1,
			waiting:1,
			can_sleep:1,
			:0;

	simple_lock_data_t	interlock;
#else	/* vax */
#ifdef	ns32000
	/*
	 *	Efficient ns32000 implementation --
	 *	see field description below.
	 */
	simple_lock_data_t	interlock;
	unsigned int	read_count:16,
			want_upgrade:1,
			want_write:1,
			waiting:1,
			can_sleep:1,
			:0;

#else	/* ns32000 */
	/*	Only the "interlock" field is used for hardware exclusion;
	 *	other fields are modified with normal instructions after
	 *	acquiring the interlock bit.
	 */
	simple_lock_data_t
			interlock;	/* Interlock for remaining fields */
	boolean_t	want_write;	/* Writer is waiting, or locked for write */
	boolean_t	want_upgrade;	/* Read-to-write upgrade waiting */
	boolean_t	waiting;	/* Someone is sleeping on lock */
	boolean_t	can_sleep;	/* Can attempts to lock go to sleep */
	int		read_count;	/* Number of accepted readers */
#endif	/* ns32000 */
#endif	/* vax */
	char		*thread;	/* Thread that has lock, if recursive locking allowed */
					/* (should be thread_t, but but we then have mutually
					   recursive definitions) */
	int		recursion_depth;/* Depth of recursion */
};

typedef struct lock	lock_data_t;
typedef struct lock	*lock_t;

#define MACH_SLOCKS	((NCPUS > 1) || MACH_LDEBUG)

#if	MACH_SLOCKS
void		simple_lock_init();
void		simple_lock();
void		simple_unlock();
boolean_t	simple_lock_try();
#else	/* MACH_SLOCKS */
/*
 *	No multiprocessor locking is necessary.
 */
#define simple_lock_init(l)
#define simple_lock(l)
#define simple_unlock(l)
#define simple_lock_try(l)	(1)	/* always succeeds */
#endif	/* MACH_SLOCKS */

/* Sleep locks must work even if no multiprocessing */

void		lock_init();
void		lock_sleepable();
void		lock_write();
void		lock_read();
void		lock_done();
boolean_t	lock_read_to_write();
void		lock_write_to_read();
boolean_t	lock_try_write();
boolean_t	lock_try_read();
boolean_t	lock_try_read_to_write();

#define	lock_read_done(l)	lock_done(l)
#define	lock_write_done(l)	lock_done(l)

void		lock_set_recursive();
void		lock_clear_recursive();

#endif	/* _LOCK_ */
