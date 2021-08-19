/*
 **********************************************************************
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
 **********************************************************************
 * HISTORY
 * 31-Jan-87  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Don't include features.h here!
 *
 *  1-Jan-87  Michael Young (mwyoung) at Carnegie-Mellon University
 *	Revamped for compilation under KERNEL switch.
 *
 **********************************************************************
 */

/*
 * adb - vax string table version; common definitions
 */

#ifdef	KERNEL
#include "kdbdefine.h"
#endif	/* KERNEL */

#ifdef	KERNEL
#include "i860/psl.h"
#include "i860/pcb.h"
#include "vm/vm_param.h"
#include "vm/vm_prot.h"
#include "i860/param.h"
#else	/* KERNEL */
#include <machine/psl.h>
#include <machine/pte.h>

#include <sys/param.h>
#include <sys/dir.h>
#include <sys/user.h>
#endif	/* KERNEL */

#ifdef DDD
#include <ctype.h>
#include <a.out.h>
#else
#include "structs.h"
#endif

#ifdef	KERNEL
#define	ADB
#define	MULD2

#undef	TRUE
#undef	FALSE
#undef	MAXFILE

#if	MACH
#ifdef	KERNEL
#include "machine/reg.h"
#include "machine/trap.h"
#include "vm/vm_map.h"
#else	/* KERNEL */
#include <vm/vm_map.h>
#endif	/* KERNEL */
#endif	/* MACH */
#endif	/* KERNEL */
#include <sys/kern_return.h>

#include "mac.h"
#include "mode.h"
#include "head.h"

/* access modes */
#define RD	0
#define WT	1

#define NSP	0
#define	ISP	1
#define	DSP	2
#define STAR	4
#define STARCOM 0200

/*
 * Symbol types, used internally in calls to findsym routine.
 * One the VAX this all degenerates since I & D symbols are indistinct.
 * Basically we get NSYM==0 for `=' command, ISYM==DSYM otherwise.
 */
#define NSYM	0
#define DSYM	1		/* Data space symbol */
#define ISYM	DSYM		/* Instruction space symbol == DSYM on VAX */

#define BKPTSET	1
#define BKPTEXEC 2

#define USERPS	PSL
#define USERPC	PC
#define BPT	0x44000000
#define TBIT	020
#define FD	0200
#define	SETTRC	0
#define	RDUSER	2
#define	RIUSER	1
#define	WDUSER	5
#define WIUSER	4
#define	RUREGS	3
#define	WUREGS	6
#define	CONTIN	7
#define	EXIT	8
#define SINGLE	9

#ifdef KERNEL
#else
/* the quantities involving ctob() are located in the kernel stack. */
/* the others are in the pcb. */
#define KSP	0
#define ESP	4
#define SSP	8
#define USP	(ctob(UPAGES)-5*sizeof(int))
#define R0	(ctob(UPAGES)-18*sizeof(int))
#define R1	(ctob(UPAGES)-17*sizeof(int))
#define R2	(ctob(UPAGES)-16*sizeof(int))
#define R3	(ctob(UPAGES)-15*sizeof(int))
#define R4	(ctob(UPAGES)-14*sizeof(int))
#define R5	(ctob(UPAGES)-13*sizeof(int))
#define R6	(ctob(UPAGES)-12*sizeof(int))
#define R7	(ctob(UPAGES)-11*sizeof(int))
#define R8	(ctob(UPAGES)-10*sizeof(int))
#define R9	(ctob(UPAGES)-9*sizeof(int))
#define R10	(ctob(UPAGES)-8*sizeof(int))
#define R11	(ctob(UPAGES)-7*sizeof(int))
#define AP	(ctob(UPAGES)-21*sizeof(int))
#define FP	(ctob(UPAGES)-20*sizeof(int))
#define PC	(ctob(UPAGES)-2*sizeof(int))
#define PSL	(ctob(UPAGES)-1*sizeof(int))
#define P0BR	80
#define P0LR	84
#define P1BR	88
#define P1LR	92
#endif

#ifdef	KERNEL
#define MAXOFF	0x1000
#else	/* KERNEL */
#define MAXOFF	255
#endif	/* KERNEL */
#define MAXPOS	80
#define MAXLIN	128
#define EOF	0
#define EOR	'\n'
#define QUOTE	0200
#define EVEN	-2

/* long to ints and back (puns) */
union {
	INT	I[2];
	long	L;
}
itolws;

#if defined(LITTLE_ENDIAN)
#define leng(a)		((long)((unsigned)(a)))
#define shorten(a)	((int)(a))
#define itol(a,b)	(itolws.I[0]=(a), itolws.I[1]=(b), itolws.L)
#else
#define leng(a)		((long)((unsigned)(a)))
#define shorten(a)	((int)(a))
#define itol(a,b)	(itolws.I[0]=(b), itolws.I[1]=(a), itolws.L)
#endif
#if 0
#define leng(a)		itol(0,a)
#define shorten(a)	((short)(a))
#define itol(a,b)	(itolws.I[0]=(b), itolws.I[1]=(a), itolws.L)
#endif

/* result type declarations */
long		inkdot();
unsigned	get();
unsigned	chkget();
string_t	exform();
long		round();
bkpt_t		scanbkpt();
VOID		fault();

struct	pcb	pcb;
int	kernel;
int	kcore;
struct	pte *sbr;
int	slr;
int	masterpcbb;

#if	MACH
struct	pcb	*curpcb;	/* pcb for selected process */
vm_map_t	curmap;		/* vm map for selected process */
int		curpid;		/* process id when entering debugger */
#endif	/* MACH */
