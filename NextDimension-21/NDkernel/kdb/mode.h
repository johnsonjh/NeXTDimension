/*
 ****************************************************************
 * Mach Operating System
 * Copyright (c) 1986 Carnegie-Mellon University
 *
 * This software was developed by the Mach operating system
 * project at Carnegie-Mellon University's Department of Computer
 * Science. Software contributors as of May 1986 include Mike Accetta,
 * Robert Baron, William Bolosky, Jonathan Chew, David Golub,
 * Glenn Marcy, Richard Rashid, Avie Tevanian and Michael Young.
 *
 * Some software in these files are derived from sources other
 * than CMU.  Previous copyright and other source notices are
 * preserved below and permission to use such software is
 * dependent on licenses from those institutions.
 *
 * Permission to use the CMU portion of this software for
 * any non-commercial research and development purpose is
 * granted with the understanding that appropriate credit
 * will be given to CMU, the Mach project and its authors.
 * The Mach project would appreciate being notified of any
 * modifications and of redistribution of this software so that
 * bug fixes and enhancements may be distributed to users.
 *
 * All other rights are reserved to Carnegie-Mellon University.
 ****************************************************************
 */
/*	mode.h	4.2	81/05/14	*/

#include "machine.h"
/*
 * sdb/adb - common definitions for old srb style code
 */

#define MAXCOM	64
#define MAXARG	32
#define LINSIZ	512

#define i860BPSTEP	1
#define i860SSTEP	2

typedef	short	INT;
typedef	int		VOID;
typedef	char		BOOL;
typedef	char		*string_t;
typedef	char		msg[];
typedef	struct map	map;
typedef	map		*map_t;
typedef	struct bkpt	bkpt;
typedef	bkpt		*bkpt_t;


/* file address maps */
struct map {
	long	b1;
	long	e1;
	long	f1;
	long	b2;
	long	e2;
	long	f2;
	INT	ufd;
};

struct bkpt {
	long	loc;
	long	ins;
	INT	count;
	INT	initcnt;
	INT	flag;
	char	comm[MAXCOM];
	bkpt	*nxtbkpt;
	char	i860_single_step;
};

typedef	struct reglist	REGLIST;
typedef	REGLIST		*REGPTR;
struct reglist {
	string_t	rname;
	INT	roffs;
	int	*rkern;
};
