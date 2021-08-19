/* 
 ************************************************************************
 * Mach Operating System
 * Copyright (c) 1987 Carnegie-Mellon University
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 ************************************************************************
 * HISTORY
 * 25-Jun-86  David Golub (dbg) at Carnegie-Mellon University
 *	Added a new trap code for KDB entry to avoid conflict with trace
 *	trap from user mode.
 *
 * 25-Feb-86  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Installed VM changes.
 *
 ************************************************************************
 */

#include "cs_kdb.h"
/*
 * Copyright (c) 1982, 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)trap.h	7.1 (Berkeley) 6/5/86
 */

/*
 * Trap type values
 */
/* The first three constant values are known to the real world <signal.h> */
#define	T_RESADFLT	0		/* reserved addressing fault */
#define	T_PRIVINFLT	1		/* privileged instruction fault */
#define	T_RESOPFLT	2		/* reserved operand fault */
/* End of known constants */
#define	T_BPTFLT	3		/* bpt instruction fault */
#define	T_XFCFLT	4		/* xfc instruction fault */
#define	T_SYSCALL	5		/* chmk instruction (syscall trap) */
#define	T_ARITHTRAP	6		/* arithmetic trap */
#define	T_ASTFLT	7		/* software level 2 trap (ast deliv) */
#define	T_SEGFLT	8		/* segmentation fault */
#define	T_PROTFLT	9		/* protection fault */
#define	T_TRCTRAP	10		/* trace trap */
#define	T_COMPATFLT	11		/* compatibility mode fault */
#define	T_PAGEFLT	12		/* page fault */
#define	T_INSFLT	13		/* instruction access fault */
#if	MACH
#define	T_READ_FAULT	14		/* read fault */
#define T_WRITE_FAULT	15		/* write fault */
#endif	/* MACH */
#if	CS_KDB
#define	T_KDB_ENTRY	16		/* force entry to kernel debugger */
#endif	/* CS_KDB */
#define T_INTRPT	17		/* hardware interrupt */

/* detail of DAT trap */
#define DAT_INV		0		/* page not present */
#define DAT_BR		1		/* data breakpoint */
#define DAT_ALN		2		/* un-aligned access */
#define DAT_DIRTY	3		/* dirty bit not set on write */
#define DAT_PERM	4		/* permissions invalid */
#define DAT_KTW		5		/* attempt to write kernel text */
#define DAT_UNKNOWN	6		/* somethings fubar */
#define DAT_UTW		7		/* attempt to write user text */
