/* 
 * Mach Operating System
 * Copyright (c) 1987 Carnegie-Mellon University
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 */
/*
 **********************************************************************
 * HISTORY
 *  3-Oct-87  Michael Young (mwyoung) at Carnegie-Mellon University
 *	De-linted.
 *
 * 16-May-86  David Golub (dbg) at Carnegie-Mellon University
 *	Support for debugging other processes than the one interrupted.
 *
 * 11-Oct-85  Robert V Baron (rvb) at Carnegie-Mellon University
 *	defined "<t"
 *
 **************************************************************************
 */

/*
 * adb - main command loop and error/interrupt handling
 */
#include "defs.h"
#include <setjmp.h>

#include "i860/proc.h"
#include "i860/reg.h"
#include "i860/trap.h"

extern		DebugInitialized;

msg		NOEOR;

INT		mkfault;
INT		executing;
INT		infile;
char		*lp;
long		maxoff;
long		maxpos;
long		sigint;
long		sigqit;
INT		wtflag;
long		maxfile;
string_t		errflg;
long		exitflg;

char		lastc;
INT		eof;

INT		lastcom;

long	maxoff = MAXOFF;
long	maxpos = MAXPOS;
#ifdef	KERNEL
long	dot;
jmp_buf	jmpbuf;
int	tregs[36];	/* reg holder if kdb called directly */
int	*db_ar0;		/* ptr to registers on stack */
#else	/* KERNEL */
char	*Ipath = "/usr/lib/adb";
#endif	/* KERNEL */

#ifdef	KERNEL
kdb(type, trapsp, curproc)
int		type;
int		*trapsp;
struct proc	*curproc;
#else	/* KERNEL */
main(argc, argv)
register char **argv;
int argc;
#endif	/* KERNEL */
{
	extern char *trap_type[];
	extern int TRAP_TYPES;
	extern char peekc;
	extern INT dotinc;
	int dr6, i, s;
#ifndef FOR_DB
	extern char symtable[];
#endif

	mkioptab();
#ifdef	KERNEL
#ifndef FOR_DB
	/* stuff symbol table address in magic place */
	*((char **)0xffffff70) = symtable;
#endif
	s = splall();
	{
		extern INT pid;

#if	MACH
		if (curproc) {

			/*
			 *	Get the map for the current process
			 */
			kdbgetprocess(curproc, &curmap, &curpcb);
			curpid = curproc->p_pid;
			var[varchk('m')] = (long) curmap;
		} else {
			/*
			 *	if there's no process...
			 */
			curmap = NULL;	/* take our chances */
			curpid = 1;	/* fake */
		}
		/*
		 *	But the pcb is the saved set of registers
		 */
		curpcb = &pcb;
		pid    = curpid;
#else	/* MACH */
		pid = 1;
#endif	/* MACH */
		var[varchk('t')] = (int)trapsp;
	}
	/* Wait for a debugger connection */
	while ( ! KernelDebugEnabled() )
		;
	DebugInitialized = 1;
	wtflag = 1;
	kcore = 1;
	setcor();
	flushbuf();
#ifdef	DEBUG
	{
		extern long loopcnt;
		printf("loop=%d\n", loopcnt);
	}
#endif	/* DEBUG */
	if ( type == 0 && trapsp == 0 )	/* Startup call */
		puts("NextDimension Kernel Debugger\n" );

	delbp();
	switch (type)
	{
	case T_BPTFLT:
		db_ar0 = trapsp;
		printf("Break => ");
common:
		dot = db_ar0[PC];
		dotinc = 0;
		psymoff(dot, ISYM, ":");
		printins(0, ISP, chkget(dot, ISP));
		break;
	case T_INSFLT:
		printf("%s Trap => ", trap_type[type]);
		dot = db_ar0[PC];
		dotinc = 0;
		psymoff(dot, ISYM, ": (Unreadable)\n");
		break;
		
	default:
		db_ar0 = trapsp;
		if ( type >= 0 && type < TRAP_TYPES )
		    printf("%s Trap => ", trap_type[type]);
		else
		    printf("Trap type %X => ", type);
		goto common;
	case 0:
		db_ar0 = tregs;
		db_ar0[FP] = get_fp();
		db_ar0[PC] = *((int *)db_ar0[FP] + 1);
		db_ar0[FP] = *(int *)db_ar0[FP];
		dot = db_ar0[PC];
		break;
	}
#ifdef DDD
	_wdr6(0);
#else
#endif
	switch (kdbsetjmp(jmpbuf)) {

	case 0:		/* first time through */
	case 1:		/* error - just continue */
		break;
	case 2:		/* continue for break point */
		setbp(db_ar0[PC]);
		return;
	case 3:		/* continue for single step */
		setss(db_ar0, i860SSTEP);
		setbp(db_ar0[PC]);
		return;

	}
#else	/* KERNEL */
another:
	if (argc>1) {
		if (eqstr("-w", argv[1])) {
			wtflag = 2;		/* suitable for open() */
			argc--, argv++;
			goto another;
		}
		if (eqstr("-k", argv[1])) {
			kernel = 1;
			argc--, argv++;
			goto another;
		}
		if (argv[1][0] == '-' && argv[1][1] == 'I') {
			Ipath = argv[1]+2;
			argc--, argv++;
		}
	}
	if (argc > 1)
		symfil = argv[1];
	if (argc > 2)
		corfil = argv[2];
	xargc = argc;
	setsym();
	setcor();
	setvar();

	if ((sigint=signal(SIGINT,SIG_IGN)) != SIG_IGN) {
		sigint = fault;
		signal(SIGINT, fault);
	}
	sigqit = signal(SIGQUIT, SIG_IGN);
	setexit();
	if (executing)
		delbp();
#endif	/* KERNEL */
	executing = 0;
	eof=0;
	peekc = 0;
	for (;;) {
		flushbuf();
		if (errflg) {
			printf("%s\n", errflg);
#if	CMU
			exitflg = (int)errflg;
#else	/* CMU */
			exitflg = errflg;
#endif	/* CMU */
			errflg = 0;
		}
		if (mkfault) {
			mkfault=0;
			printc('\n');
			prints(DBNAME);
		}
		lp=0;
		puts("GAcK> ");
		rdc();
		lp--;
		if (eof) {
#ifdef	KERNEL
			backall(s);
			return(1);
#else	/* KERNEL */
			if (infile) {
				iclose(-1, 0);
				eof=0;
				reset();
			} else
				done();
#endif	/* KERNEL */
		} else
			exitflg = 0;
		command(0, lastcom);
		if (lp && lastc!='\n')
			error(NOEOR);
	}
}

#ifdef	KERNEL
#else	/* KERNEL */
done()
{
	endpcs();
	printf( "Done, status %d\n", exitflg );
	exit(exitflg);
}
#endif	/* KERNEL */

long
round(a,b)
register long a, b;
{
	register long w;
	w = (a/b)*b;
	if ( a!=w ) {
		w += b;
	}
	return(w);
}

/*
 * If there has been an error or a fault, take the error.
 */
chkerr()
{
	if (errflg || mkfault)
		error(errflg);
}

/*
 * An error occurred; save the message for later printing,
 * close open files, and reset to main command loop.
 */
error(n)
char *n;
{
	errflg = n;
	iclose(0, 1);
	oclose();
#ifdef	KERNEL
	kdblongjmp(jmpbuf, 1);
#else	/* KERNEL */
	reset();
#endif	/* KERNEL */
}

#ifdef	KERNEL
#else	/* KERNEL */
/*
 * An interrupt occurred; reset the interrupt
 * catch, seek to the end of the current file
 * and remember that there was a fault.
 */
fault(a)
{
	signal(a, fault);
	lseek(infile, 0L, 2);
	mkfault++;
}
#endif	/* KERNEL */

/*************************************************************************/
extern	bkpt_t bkpthead;

setss(trapsp, step_type)
int	*trapsp;
char	step_type;

{
	int program_counter, faulting_instr, psw, next_step, 
	xx, lbroff, src1ni, src1immediate, src2, sbroff;
	bkpt_t ssbkptr; 

	program_counter = trapsp[PC]; 
	faulting_instr = *(int *)((long)program_counter ^ 4);	/* Little-big conv */
	psw = trapsp[PSR];

/* printf("setss(): pc = 0x%X fault_ins = 0x%X, psw = 0x%X\n",
		program_counter, faulting_instr, psw );
*/
	next_step = program_counter + 4;

	lbroff = (int)(faulting_instr & 0x03ffffff);	
	lbroff = (lbroff << 2);
	if (lbroff & 0x08000000) lbroff |= 0xf0000000;

	sbroff = (faulting_instr & 0x001f0000) >> 5;
	sbroff |=(faulting_instr & 0x000007ff);
	sbroff = sbroff <<2;
	if (sbroff & 0x00020000) sbroff |= 0xfffc0000;

	src1immediate = (faulting_instr & 0x0000f800) >> 11;
	src1ni = trapsp[src1immediate + 4];

	if (src1immediate & 0x10)	/* sign extend */
	src1immediate |= 0xffffffe0;

	src2 = ((faulting_instr & 0x03e00000) >> 11) >> 10;
	src2 = trapsp[src2 + 4];

	/* check for CTRL format instruction */

	if ((faulting_instr & 0xe0000000) == 0x60000000)
    	{
		xx = 1;
		switch (((int)(faulting_instr >> 16) >> 10) & 7)
		{
			case 2: /* br   */
			case 3: /* call */
				next_step += lbroff;
				break;

			case 4: /* bc */
				if (psw & PSR_CC) next_step += lbroff;
				break;

			case 5: /* bc.t */
				if (psw & PSR_CC) next_step += lbroff;
				else	next_step += 4;
				break;
		
			case 6: /* bnc */
				if ( !(psw & PSR_CC) ) next_step += lbroff;
				break;

			case 7: /* bnc.t */
				if ( !(psw & PSR_CC) ) next_step += lbroff;
				else	next_step += 4;
				break;
		}
	}

	if ((faulting_instr & 0xfc000000) == 0x40000000) 	/* bri instr */
		next_step = src1ni;

	if ((faulting_instr & 0xfc000000) == 0xb4000000) 	/* bla instr */
		if (psw & PSR_LCC)
		next_step += sbroff;
		else	next_step += 4;

	if ((faulting_instr & 0xf8000000) == 0x50000000)	/* btne instr*/
	{
		if (faulting_instr & 0x04000000)		/* immed. bit */
			if(src1immediate != src2)
				next_step += sbroff;
		else
			if(src1ni != src2)
				next_step += sbroff;
	}
	if ((faulting_instr & 0xf8000000) == 0x58000000)	/* bte instr*/
	{
		if (faulting_instr & 0x04000000)		/* immed. bit */
			if(src1immediate == src2)
				next_step += sbroff;
		else
			if(src1ni == src2)
				next_step += sbroff;
	}


	if ( (ssbkptr=scanbkpt(next_step)) ) {
		ssbkptr->flag=0;
	}
	for ( ssbkptr=bkpthead; ssbkptr; ssbkptr=ssbkptr->nxtbkpt ) {
		if ( ssbkptr->flag == 0 ) {
			break;
		}
	}
	if ( ssbkptr==0 ) {
		if ( (ssbkptr=(bkpt_t)sbrk(sizeof *ssbkptr)) == (bkpt_t)-1 )
			{
				error( 0   /*SZBKPT*/ );
			} else {
				ssbkptr->nxtbkpt=bkpthead;
				bkpthead=ssbkptr;
			}
	}
/*
printf("setss(): next_step = 0x%X\n", next_step);
 */
	ssbkptr->loc = next_step;
	ssbkptr->initcnt = ssbkptr->count = 1;
	ssbkptr->flag = BKPTSET;

	/* if resuming from a breakpoint (as opposed to a single step), 
	this breakpoint will be thrown away in delbp() */

	ssbkptr->i860_single_step = step_type;
}

