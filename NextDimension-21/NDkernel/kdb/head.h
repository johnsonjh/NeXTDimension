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
 *
 * HISTORY
 * 31-Jan-87  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Don't define u if compiling in the kernel.
 *
 */
/*	head.h	4.1	81/05/14	*/

long	maxoff;
long	localval;

struct	nlist *symtab, *esymtab;
struct	nlist *cursym;
struct	nlist *lookup();

#ifdef KERNEL
#else
struct	exec filhdr;
#endif

long	var[36];

int	xargc;

map	txtmap;
map	datmap;
INT	wtflag;
INT	fcor;
INT	fsym;
long	maxfile;
long	maxstor;
INT	signo;

#ifdef	KERNEL
#else	/* KERNEL */
union {
	struct	user U;
	char	UU[ctob(UPAGES)];
}
udot;
#define	u	udot.U
#endif	/* KERNEL */

char	*corfil, *symfil;

