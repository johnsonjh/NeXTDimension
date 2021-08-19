/* 
 * Mach Operating System
 * Copyright (c) 1987 Carnegie-Mellon University
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 */
#ifdef  KERNEL
/*****************************************************************
 * HISTORY
 *  3-Dec-86  Michael Young (mwyoung) at Carnegie-Mellon University
 *	Added "single step until matching return" form of ":j".
 *
 *  3-Oct-86  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Continue kernel execution even if pid==0.
 *
 * 6-jan-86	David Golub (dbg)
 *  new commands:
 *	:j single-steps until CALL[S,G] or RET encountered,
 *         printing PC at each stop
 *      :p like :s, but prints PC if count not done yet
 *
 ****************************************************************/
#endif	/* KERNEL */

/*
 *
 *	UNIX debugger
 *
 */

#include "defs.h"
#include "pcs.h"
#include "i860/reg.h"
#include <setjmp.h>

msg		NOBKPT;
msg		SZBKPT;
msg		EXBKPT;
msg		NOPCS;
msg		BADMOD;

/* breakpoints */
bkpt_t		bkpthead;

char		*lp;
char		lastc;

INT		signo;
long		dot;
INT		pid;
long		cntval;
long		loopcnt;

long		entrypt;
INT		adrflg;

/* sub process control */

subpcs(modif)
{
	extern jmp_buf	jmpbuf;
	extern int	*db_ar0;
	extern long	expv;
	register INT		check;
	INT		execsig,runmode;
	register bkpt_t	bkptr;
	string_t		comptr;
	char	c;
	int	i, shift, dr6, dr7;

	execsig=0;
	loopcnt=cntval;

	switch (modif) {

		/* delete breakpoint */
	case 'D':
		DataTrap( 0, 0 );	/* Disable the data access trap */
		db_ar0[DB] = 0;
		db_ar0[PSR] &= ~3;
		return;
		
	case 'd':
		if ( (bkptr=scanbkpt(dot)) ) {
			bkptr->flag=0;
			return;
		} else {
			error(NOBKPT);
		}

		/* set breakpoint */
	case 'B':
		DataTrap( dot, 2 );	/* Break on a write to dot */
		db_ar0[DB] = dot;
		db_ar0[PSR] |= 2;
		return;
		
	case 'b':
		if ( (bkptr=scanbkpt(dot)) ) {
			bkptr->flag=0;
		}
		for ( bkptr=bkpthead; bkptr; bkptr=bkptr->nxtbkpt ) {
			if ( bkptr->flag == 0 ) {
				break;
			}
		}
		if ( bkptr==0 ) {
#if	CMU
			if ( (bkptr=(bkpt_t)sbrk(sizeof *bkptr)) == (bkpt_t)-1 )
#else	/* CMU */
				if ( (bkptr=sbrk(sizeof *bkptr)) == -1 )
#endif	/* CMU */
				{
					error(SZBKPT);
				} else {
					bkptr->nxtbkpt=bkpthead;
					bkpthead=bkptr;
				}
		}
		bkptr->loc = dot;
		bkptr->initcnt = bkptr->count = cntval;
		bkptr->flag = BKPTSET;
		bkptr->i860_single_step = 0;
		check=MAXCOM-1;
		comptr=bkptr->comm;
		rdc();
		lp--;
		do {
			*comptr++ = readchar();
		}
		while( check-- && lastc!=EOR ) ;
		*comptr=0;
		lp--;
		if ( check ) {
			return;
		} else {
			error(EXBKPT);
		}

#ifdef	KERNEL
#else	/* KERNEL */
		/* exit */
	case 'k' :
	case 'K':
		if ( pid ) {
			printf("%d: killed", pid);
			endpcs();
			return;
		}
		error(NOPCS);

		/* run program */
	case 'r':
	case 'R':
		endpcs();
		setup();
		runmode=CONTIN;
		if ( adrflg ) {
			if ( !scanbkpt(dot) ) {
				loopcnt++;
			}
		} else {
			if ( !scanbkpt(entrypt+2) ) {
				loopcnt++;
			}
		}
		break;
#endif	/* KERNEL */

		/* single step */
	case 's':
	case 'S':
#ifdef	KERNEL
#ifndef i860
		db_ar0[EFL] |= EFL_TF;
#endif
		kdblongjmp(jmpbuf, 3);
#else	/* KERNEL */
		if ( pid ) {
			runmode=SINGLE;
			execsig=getsig(signo);
			sstepmode = STEP_NORM;
		} else {
			setup();
			loopcnt--;
		}
#endif	/* KERNEL */
		break;

#ifdef	KERNEL
	case 'p':
	case 'P':

		runmode = SINGLE;
		execsig = getsig(signo);
		sstepmode = STEP_PRINT;
		break;

	case 'j':
	case 'J':

		runmode = SINGLE;
		execsig = getsig(signo);
		sstepmode = isupper(modif) ? STEP_RETURN : STEP_CALLT;
		call_depth = 1;
		icount = 0;
		break;
#endif	/* KERNEL */

		/* continue with optional signal */
	case 'c':
	case 'C':
	case 0:
#ifdef	KERNEL
		kdblongjmp(jmpbuf, 2);
#else	/* KERNEL */
		if ( pid==0 ) {
			error(NOPCS);
		}
		runmode=CONTIN;
		execsig=getsig(signo);
		sstepmode = STEP_NONE;
#endif	/* KERNEL */
		break;

	default:
		error(BADMOD);
	}

#ifdef	KERNEL
	if ( loopcnt>0 ) {
		runpcs(runmode,0);
	}
#ifdef	lint
	runpcs(runmode, execsig);	/* reference execsig */
#endif	/* lint */
#else	/* KERNEL */
	if ( loopcnt>0 && runpcs(runmode,execsig) ) {
		printf("breakpoint%16t");
	} else {
		printf("stopped at%16t");
	}
	delbp();
	printpc();
#endif	/* KERNEL */
}
