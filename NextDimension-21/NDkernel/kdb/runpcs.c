/* 
 * Mach Operating System
 * Copyright (c) 1987 Carnegie-Mellon University
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 */

#undef	DEBUG

#ifdef	KERNEL
/*****************************************************************
 * HISTORY
 *  3-Oct-87  Michael Young (mwyoung) at Carnegie-Mellon University
 *	De-linted.
 *
 * 27-May-87  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	sigcode -> kdbsigcode
 *
 *  3-Dec-86  Michael Young (mwyoung) at Carnegie-Mellon University
 *	Added "single step until matching return" form of ":j".
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
#include <signal.h>

extern	map	txtmap;

msg		NOFORK;
msg		ENDPCS;
msg		BADWAIT;

char		*lp;
long		sigint;
long		sigqit;

/* breakpoints */
extern bkpt_t		bkpthead;

REGLIST		reglist[];

char		lastc;

INT		fcor;
INT		fsym;
string_t	errflg;
INT		errno;
INT		signo;
INT		kdbsigcode;

long		dot;
string_t	symfil;
INT		wtflag;
INT		pid;
long		expv;
INT		adrflg;
long		loopcnt;

#ifdef	KERNEL
#include	"pcs.h"
#endif	/* KERNEL */



/* service routines for sub process control */

getsig(sig)
{
	return(expr(0) ? expv : sig);
}

long userpc = 1;

runpcs(runmode,execsig)
{
#if	CS_LINT
#else	/* CS_LINT */
	INT		rc;
#endif	/* CS_LINT */
	register bkpt_t	bp;
	if ( adrflg ) {
		userpc=dot;
	}
#ifdef	KERNEL
#else	/* KERNEL */
	printf("%s: running\n", symfil);

	while ( --loopcnt>=0 ) {
#endif	/* KERNEL */
	    if (execsig == 0)
		printf("kernel: running\n");
#ifdef DEBUG
		printf("\ncontinue %x %d\n",userpc,execsig);
#endif
		/* can this runmode == single be tossed on i860 ?? ww 89.1.22 */
		if ( runmode==SINGLE ) {
			delbp(); /* hardware handles single-stepping */
		} else { /* continuing from a breakpoint is hard */
			if ( bp=scanbkpt(userpc) ) {
				execbkpt(bp,execsig);
				execsig=0;
			}
			setbp(0);
		}
#ifndef	KERNEL
		ptrace(runmode,pid,userpc,execsig);
		bpwait();
		chkerr();
		execsig=0;
		delbp();
		readregs();
#else	/* KERNEL */
#ifdef	DEBUG
		printf("reset(%d) from runpcs()\n", runmode);
#endif	/* DEBUG */
		reset(runmode);
}

INT execbkptf = 0;

	/*
	 * determines whether to stop, and what to print if so
	 * flag:	1 if entered by trace trap
	 * execsig:	(seems not to be used by kernel debugger)
	 *
	 * exits:
	 *	skipping breakpoint (execbkptf != 0):
	 *		runpcs(CONTIN)
	 *      next iteration of breakpoint:
	 *		runpcs(CONTIN)
	 *	next iteration of single-step:
	 *		runpcs(SINGLE)
	 *
	 *	stopped by breakpoint:
	 *		returns 1
	 *	stopped by single-step, or
	 *		by CALL/RET:
	 *		returns 0
	 *
	 *	normal return MUST reset sstepmode!
	 */

nextpcs(flag, execsig)
{
		INT		rc;
		register bkpt_t	bp;

#ifdef HANS
		pcb.pcb_tss.t_eflags &= ~PSL_T;
#endif
		signo = flag?SIGTRAP:0;
		delbp();
		if (execbkptf) {
			execbkptf = 0;
			runpcs(CONTIN, 1);
		}
#endif	/* KERNEL */

		if ( (signo==0) && (bp=scanbkpt(userpc)) ) {
		     /* stopped by BPT instruction */
#ifdef DEBUG
			printf("\n BPT code; '%s'%o'%o'%d",
			bp->comm,bp->comm[0],EOR,bp->flag);
#endif
			dot=bp->loc;
			if ( bp->flag==BKPTEXEC
			    || ((bp->flag=BKPTEXEC)
			    && bp->comm[0]!=EOR
			    && command(bp->comm,':')
			    && --bp->count) ) {
#ifdef	KERNEL
				loopcnt++;
				execbkpt(bp,execsig);
				execsig=0;
#else	/* KERNEL */
				execbkpt(bp,execsig);
				execsig=0;
				loopcnt++;
#endif	/* KERNEL */
			} else {
				bp->count=bp->initcnt;
				rc=1;
			}
		} else {
			execsig=signo;
			rc=0;
		}
#ifndef	KERNEL
	}
#else	/* KERNEL */
	if (--loopcnt > 0) {
		if (sstepmode == STEP_PRINT){
			printf("%16t");
			printpc();
		}
		runpcs(rc?CONTIN:SINGLE, 1);
	}
	if (sstepmode == STEP_RETURN) {
		/*
		 *	Keep going until matching return
		 */

		int	ins = chkget(dot, ISP) & 0xff;

		if ((ins == I_REI) || ((ins == I_RET) && (--call_depth == 0)))
			printf("%d instructions executed\n", icount);
		else {
			if ((ins == I_CALLS) || (ins == I_CALLG) || (ins == I_RET)) {
				register int i;

				printf("[after %6d]     ", icount);
				for (i = call_depth; --i > 0;)
					printf("  ");
				printpc();
			}
			if ((ins == I_CALLS) || (ins == I_CALLG))
				call_depth++;

			loopcnt++;
			icount++;
			runpcs(SINGLE, 1);
		}
	}
	if (sstepmode == STEP_CALLT){
		/*
		 *	Keep going until CALL or RETURN
		 */

		int	ins = chkget(dot,ISP) & 0xff;

		if ((ins == I_CALLS) || (ins == I_CALLG) || (ins == I_RET) || (ins == I_REI))
			printf("%d instructions executed\n", icount);
		else {
			loopcnt++;
			icount++;
			runpcs(SINGLE, 1);
		}
	}
	sstepmode = STEP_NONE;	/* don't wait for CALL/RET */
#endif	/* KERNEL */
	return(rc);
}

#define BPOUT 0
#define BPIN 1
INT bpstate = BPOUT;

#ifdef	KERNEL
#else	/* KERNEL */
endpcs()
{
	register bkpt_t	bp;
	if ( pid ) {
		ptrace(EXIT,pid,0,0);
		pid=0;
		userpc=1;
		for ( bp=bkpthead; bp; bp=bp->nxtbkpt ) {
			if ( bp->flag ) {
				bp->flag=BKPTSET;
			}
		}
	}
	bpstate=BPOUT;
}
#endif	/* KERNEL */

#ifdef VFORK
nullsig()
{

}
#endif

#ifdef	KERNEL
#else	/* KERNEL */
setup()
{
	close(fsym);
	fsym = -1;
#ifndef VFORK
	if ( (pid = fork()) == 0 )
#else
		if ( (pid = vfork()) == 0 )
#endif
		{
			ptrace(SETTRC,0,0,0);
#ifdef VFORK
			signal(SIGTRAP,nullsig);
#endif
			signal(SIGINT,sigint);
			signal(SIGQUIT,sigqit);
			doexec();
			exit(0);
		} 
		else if ( pid == -1 ) {
			error(NOFORK);
		} 
		else {
			bpwait();
			readregs();
			lp[0]=EOR;
			lp[1]=0;
			fsym=open(symfil,wtflag);
			if ( errflg ) {
				printf("%s: cannot execute\n",symfil);
				endpcs();
				error(0);
			}
		}
	bpstate=BPOUT;
}
#endif	/* KERNEL */

execbkpt(bp,execsig)
bkpt_t	bp;
{
#ifdef DEBUG
	printf("exbkpt: %d\n",bp->count);
#endif
	delbp();
#ifdef	KERNEL
	bp->flag=BKPTSET;
	execbkptf++;
#ifdef	DEBUG
	printf("reset(%d) from execbkpt()\n", SINGLE);
#endif	/* DEBUG */
	reset(SINGLE);
#ifdef	lint
	if (execsig++) panic("");
#endif	/* lint */
#else /*  KERNEL */
	ptrace(SINGLE,pid,bp->loc,execsig);
	bp->flag=BKPTSET;
	bpwait();
	chkerr();
	readregs();
#endif	/* KERNEL */
}


#ifdef	KERNEL
#else	/* KERNEL */
doexec()
{
	string_t		argl[MAXARG];
	char		args[LINSIZ];
	string_t		p, *ap, filnam;
	extern string_t environ;
	ap=argl;
	p=args;
	*ap++=symfil;
	do {
		if ( rdc()==EOR ) {
			break;
		}
		*ap = p;

		/*
		 * Store the arg if not either a '>' or '<'
		 */
		while ( lastc!=EOR && lastc!=' ' && lastc!='	' && lastc!='>' && lastc!='<'							/* slr001 */ ) {
			*p++=lastc;
			readchar();
		}		/* slr001 */
		*p++ = '\0';					/* slr001 */
		ap++;						/* slr001 */

		/*
		 * First thing is to look for direction characters
		 * and get filename.  Do not use up the args for filenames.
		 */
		if ( lastc=='<' ) {
			do {
				readchar();
			}
			while( lastc==' ' || lastc=='	' ) ;
			filnam = p;
			while ( lastc!=EOR && lastc!=' ' && lastc!='	' && lastc!='>' ) {
				*p++=lastc;
				readchar();
			}
			*p = 0;
			close(0);
			if ( open(filnam,0)<0 ) {
				printf("%s: cannot open\n",filnam);
				_exit(0);
			}
			p = *--ap; /* slr001 were on arg ahead of ourselves */
		} else if ( lastc=='>' ) {
			do {
				readchar();
			}
			while( lastc==' ' || lastc=='	' ) ;
			filnam = p;
			while ( lastc!=EOR && lastc!=' ' && lastc!='	' && lastc!='<' ) {
				*p++=lastc;
				readchar();
			}
			*p = '\0';
			close(1);
			if ( creat(filnam,0666)<0 ) {
				printf("%s: cannot create\n",filnam);
				_exit(0);
			}
			p = *--ap; /* slr001 were on arg ahead of ourselves */
		}

	}
	while( lastc!=EOR ) ;
	*ap++=0;
	exect(symfil, argl, environ);
	perror(symfil);
}
#endif	/* KERNEL */

bkpt_t	scanbkpt(adr)
long adr;
{
	register bkpt_t	bp;
	for ( bp=bkpthead; bp; bp=bp->nxtbkpt ) {
		if ( bp->flag && bp->loc==adr ) {
			break;
		}
	}
	return(bp);
}

delbp()
{
	register long	a;
	register bkpt_t	bp;
	char xx = 0;
	if ( bpstate!=BPOUT ) {
		for ( bp=bkpthead; bp; bp=bp->nxtbkpt ) {
			if ( bp->flag ) {
				a=bp->loc;
				if (xx = bp->i860_single_step) {
					/*this bkpt was inserted only for a :s*/
					bp->flag = 0;	
					/* clean up trail of old :s bkpt's */
					bp->i860_single_step = 0; 
				}
#ifdef	KERNEL
				put(a, ISP, bp->ins);
#else	/* KERNEL */
				if ( a < txtmap.e1 ) {
					ptrace(WIUSER,pid,a,
					(bp->ins&0xFF)|(ptrace(RIUSER,pid,a,0)&~0xFF));
				} else {
					ptrace(WDUSER,pid,a,
					(bp->ins&0xFF)|(ptrace(RDUSER,pid,a,0)&~0xFF));
				}
#endif	/* KERNEL */
			}
		}
		bpstate=BPOUT;
	}
	return(xx);
}

setbp(current_pc)
long current_pc;
{
	register long		a;
	register unsigned long	insn;
	register bkpt_t	bp;

	if ( bpstate!=BPIN ) {
		for ( bp=bkpthead; bp; bp=bp->nxtbkpt ) {
			if ( (bp->flag) && (current_pc != bp->loc) ) {
				a = bp->loc;

/* printf("setbp(): bp->loc = 0x%X\n", bp->loc); */

				insn = get(a, ISP);
				/* Don't put bkpt on float half in DIM! */
				if ( (insn & 0xFC000200) == 0x48000200 ||
				     insn == 0xB0000200 ) 
				{
					a += 4;
					insn = get(a, ISP);
				}
				bp->ins = insn;
				put(a, ISP, BPT);
				if ( errno ) {
					prints("cannot set breakpoint: ");
					psymoff(bp->loc,ISYM,"\n");
				}
			}
		}
		bpstate=BPIN;
	}
}

#ifdef	KERNEL
#else	/* KERNEL */
bpwait()
{
	register long w;
	long stat;

	signal(SIGINT, 1);
	while ( (w = wait(&stat))!=pid && w != -1 ) ;
	signal(SIGINT,sigint);
	if ( w == -1 ) {
		pid=0;
		errflg=BADWAIT;
	} else if ( (stat & 0177) != 0177 ) {
		kdbsigcode = 0;
		if ( signo = stat&0177 ) {
			sigprint();
		}
		if ( stat&0200 ) {
			prints(" - core dumped");
			close(fcor);
			setcor();
		}
		pid=0;
		errflg=ENDPCS;
	} else {
		signo = stat>>8;
		kdbsigcode = ptrace(RUREGS, pid, &((struct user *)0)->u_code, 0);
		if ( signo!=SIGTRAP ) {
			sigprint();
		} else {
			signo=0;
		}
		flushbuf();
	}
}

readregs()
{
	/*get register values from pcs*/
	register i;
	for ( i=24; --i>=0; ) {
		*(long *)(((long)&u)+reglist[i].roffs) =
		    ptrace(RUREGS, pid, reglist[i].roffs, 0);
	}
	userpc= *(long *)(((long)&u)+PC);
}
#endif	/* KERNEL */
