/* 
 * Mach Operating System
 * Copyright (c) 1987 Carnegie-Mellon University
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 */
/*
 **********************************************************************
 * HISTORY
 *  8-Dec-87  David Golub (dbg) at Carnegie-Mellon University
 *	Changed $K to print specified thread's stack (<thread>$K).
 *
 *  5-Oct-87  David Golub (dbg) at Carnegie-Mellon University
 *	Changed thread state printout to accommodate new scheduling
 *	state machine.
 *
 *  3-Oct-87  Michael Young (mwyoung) at Carnegie-Mellon University
 *	De-linted.
 *
 * 27-May-87  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	sigcode -> kdbsigcode.
 *
 * 15-May-87  Robert Baron (rvb) at Carnegie-Mellon University
 *	Have $z go to the mach_db debugger.
 *
 * 14-Mar-87  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Better support for MACH_TT.
 *
 * 31-Jan-87  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Preliminary support for MACH_TT in $l command.
 *
 *  7-Nov-86  David Golub (dbg) at Carnegie-Mellon University
 *	Made $l command print 'W' for swapped-out processes.
 *
 *  8-Jul-86  David Golub (dbg) at Carnegie-Mellon University
 *	Added $k (old format) and $K (new format) to trace only the
 *	kernel stack.
 *
 * 16-May-86  David Golub (dbg) at Carnegie-Mellon University
 *	Support for multiple-process debugging
 *
 * 11-Oct-85  Robert V Baron (rvb) at Carnegie-Mellon University
 *	Flushed the stupid #
 **********************************************************************
 */

#define MACH_DB 0

/*
 *
 *	UNIX debugger
 *
 */
#include "defs.h"



#ifdef	KERNEL
#include "i860/proc.h"
#include "i860/reg.h"
#endif	/* KERNEL */
#include <signal.h>

msg		LONGFIL;
msg		NOTOPEN;
msg		A68BAD;
msg		A68LNK;
msg		BADMOD;

map		txtmap;
map		datmap;

long		lastframe;
long		callpc;

INT		infile;
INT		outfile;
char		*lp;
long		maxoff;
long		maxpos;
INT		radix;

/* symbol management */
long		localval;

/* breakpoints */
bkpt_t		bkpthead;

extern int	*db_ar0;

REGLIST reglist [] = {
	"r4", R4, 0,
	"r5", R5, 0,
	"r6", R6, 0,
	"r7", R7, 0,
	"r8", R8, 0,
	"r9", R9, 0,
	"r10", R10, 0,
	"r11", R11, 0,
	"r12", R12, 0,
	"r13", R13, 0,
	"r14", R14, 0,
	"r15", R15, 0,
	"r16", R16, 0,
	"r17", R17, 0,
	"r18", R18, 0,
	"r19", R19, 0,
	"r20", R20, 0,
	"r21", R21, 0,
	"r22", R22, 0,
	"r23", R23, 0,
	"r24", R24, 0,
	"r25", R25, 0,
	"r26", R26, 0,
	"r27", R27, 0,
	"r28", R28, 0,
	"r29", R29, 0,
	"r30", R30, 0,
	"r31", R31, 0,
	"r1", R1, 0,
	"sp", SP, 0,
	"fp", FP, 0,
	"db", DB, 0,
	"psr",  PSR,	0,
	"pc", PC, 0,
};

#define NREGS	(sizeof(reglist)/sizeof (REGLIST))

REGLIST freglist [] = {
	"f0", FREGS+0, 0,
	"f1", FREGS+1, 0,
	"f2", FREGS+2, 0,
	"f3", FREGS+3, 0,
	"f4", FREGS+4, 0,
	"f5", FREGS+5, 0,
	"f6", FREGS+6, 0,
	"f7", FREGS+7, 0,
	"f8", FREGS+8, 0,
	"f9", FREGS+9, 0,
	"f10", FREGS+10, 0,
	"f11", FREGS+11, 0,
	"f12", FREGS+12, 0,
	"f13", FREGS+13, 0,
	"f14", FREGS+14, 0,
	"f15", FREGS+15, 0,
	"f16", FREGS+16, 0,
	"f17", FREGS+17, 0,
	"f18", FREGS+18, 0,
	"f19", FREGS+19, 0,
	"f20", FREGS+20, 0,
	"f21", FREGS+21, 0,
	"f22", FREGS+22, 0,
	"f23", FREGS+23, 0,
	"f24", FREGS+24, 0,
	"f25", FREGS+25, 0,
	"f26", FREGS+26, 0,
	"f27", FREGS+27, 0,
	"f28", FREGS+28, 0,
	"f29", FREGS+29, 0,
	"f30", FREGS+30, 0,
	"f31", FREGS+31, 0
};

#define NFREGS	(sizeof(freglist)/sizeof (REGLIST))

REGLIST pipereglist [] = {
	"A1L",  PSV_A1, 0,
	"A1H",  PSV_A1 + 1, 0,
	"A2L",  PSV_A2, 0,
	"A2H",  PSV_A2 + 1, 0,
	"A3L",  PSV_A3, 0,
	"A3H",  PSV_A3 + 1, 0,
	"M1L",  PSV_M1, 0,
	"M1H",  PSV_M1 + 1, 0,
	"M2L",  PSV_M2, 0,
	"M2H",  PSV_M2 + 1, 0,
	"M3L",  PSV_M3, 0,
	"M3H",  PSV_M3 + 1, 0,
	"L1L",  PSV_L1, 0,
	"L1H",  PSV_L1 + 1, 0,
	"L2L",  PSV_L2, 0,
	"L2H",  PSV_L2 + 1, 0,
	"L3L",  PSV_L3, 0,
	"L3H",  PSV_L3 + 1, 0,
	"I1L",  PSV_I1, 0,
	"I1H",  PSV_I1 + 1, 0,
	"KIL",  SPC_KI, 0,
	"KIH",  SPC_KI + 1, 0,
	"KRL",  SPC_KR, 0,
	"KRH",  SPC_KR + 1, 0,
	"TL",   SPC_T, 0,
	"TH",   SPC_T + 1, 0,
	"MRGL",	SPC_MERGE,0,
	"MRGH",	SPC_MERGE + 1,0,
	"FSR1",	PSV_FSR1, 0,
	"FSR2",  PSV_FSR2, 0,
	"FSR3",  PSV_FSR, 0
};

#define NPIPEREGS	(sizeof(pipereglist)/sizeof (REGLIST))

char		lastc;

INT		fcor;
string_t		errflg;
INT		signo;
INT		kdbsigcode;


long		dot;
long		var[];
string_t	symfil;
string_t	corfil;
INT		pid;
long		adrval;
INT		adrflg;
long		cntval;
INT		cntflg;

string_t	signals[] = {
	"",
	"hangup",
	"interrupt",
	"quit",
	"illegal instruction",
	"trace/BPT",
	"IOT",
	"EMT",
	"floating exception",
	"killed",
	"bus error",
	"memory fault",
	"bad system call",
	"broken pipe",
	"alarm call",
	"terminated",
	"signal 16",
	"stop (signal)",
	"stop (tty)",
	"continue (signal)",
	"child termination",
	"stop (tty input)",
	"stop (tty output)",
	"input available (signal)",
	"cpu timelimit",
	"file sizelimit",
	"signal 26",
	"signal 27",
	"signal 28",
	"signal 29",
	"signal 30",
	"signal 31",
};

/* general printing routines ($) */

printtrace(modif)
{
#ifdef	KERNEL
	extern long	expv;
	INT		i;
	register bkpt_t	bkptr;
	int		dr7;
	string_t	comptr;
	register struct nlist *sp;
#else	/* KERNEL */
	INT		narg, i, stat, name, limit;
	unsigned	dynam;
	register bkpt_t	bkptr;
	char		hi, lo;
	long		word;
	string_t	comptr;
	long		argp, frame, link;
	register struct nlist *sp;
	INT		stack;
	INT		ntramp;
#endif	/* KERNEL */

	if ( cntflg==0 ) {
		cntval = -1;
	}

	switch (modif) {

#ifdef	KERNEL
#if	MACH_DB
	case 'z':
		mdb_init();
		break;

#endif	/* MACH_DB */
#if	MACH
#if 0
	case 'p':
		/*
		 *	pid$p	sets process to pid
		 *	$p	sets process to the one that invoked
		 *		kdb
		 */
		{
			struct proc 	*p;
			extern struct proc *	pfind();

			extern void	kdbgetprocess();

			if (adrflg) {
				p = pfind(dot);
			} else {
				p = pfind(curpid);	/* pid at entry */
			}
			if (p) {
				printf("proc entry at 0x%X\n", p);
				kdbgetprocess(p, &curmap, &curpcb);
				var[varchk('m')] = (long) curmap;
				pid = p->p_pid;
				if (pid == curpid) {
					/*
					 * pcb for entry process is saved
					 * register set
					 */
					curpcb = &pcb;
				}
			} else {
				printf("no such process");
			}
		}
		break;
#endif
	case 'l':
		/*
		 *	print list of all active threads
		 */
	{
		thread_t	thread;
		task_t		task;

		task = (task_t) queue_first(&all_tasks);
		while (!queue_end(&all_tasks, (queue_entry_t) task)) {
			printf("task 0x%X: ", task);
			if (task->thread_count == 0) {
				printf("no threads\n");
			}
			else if (task->thread_count > 1) {
				printf("%d threads:\n",
						task->thread_count);
			}
			thread = (thread_t) queue_first(
					&task->thread_list);
			while (!queue_end(&task->thread_list,
					(queue_entry_t) thread)) {
				printf("thread 0x%X ", thread);
				printf(" %c%c%c%c%s",
				(thread->state & TH_RUN)  ? 'R' : ' ',
				(thread->state & TH_WAIT) ? 'W' : ' ',
				(thread->state & TH_SUSP) ? 'S' : ' ',
				(!thread->interruptible)  ? 'N' : ' ',
				(thread->state & TH_SWAPPED) ?
					"(swapped)" : "");
				if (thread->state & TH_WAIT ) {
					printf(" ");
					psymoff(thread->wait_event,
						ISYM, "");
				}
				else {
					time_value_t ut, st;

					thread_read_times(thread, &ut, &st);
					printf(" pri = %D, %Du %Ds %Dt %Dc",
						thread->sched_pri,
						ut.seconds,
						st.seconds,
						ut.seconds + st.seconds,
						thread->cpu_usage);
				}
				printc(EOR);
				thread = (thread_t)
					queue_next(&thread->thread_list);
			}
			task = (task_t) queue_next(&task->all_tasks);
		}
	}
#endif	/* MACH */
#else	/* KERNEL */
	case '<':
		if ( cntval == 0 ) {
			while ( readchar() != EOR ) {
			}
			lp--;
			break;
		}
		if ( rdc() == '<' ) {
			stack = 1;
		} else {
			stack = 0;
			lp--;
		}
		/* fall through */
	case '>':
		{
			char		file[64];
			char		Ifile[128];
			extern char	*Ipath;
			INT		index;

			index=0;
			if ( rdc()!=EOR ) {
				do {
					file[index++]=lastc;
					if ( index>=63 ) {
						error(LONGFIL);
					}
				}
				while( readchar()!=EOR ) ;
				file[index]=0;
				if ( modif=='<' ) {
					if ( Ipath ) {
						strcpy(Ifile, Ipath);
						strcat(Ifile, "/");
						strcat(Ifile, file);
					}
					if ( strcmp(file, "-")!=0 ) {
						iclose(stack, 0);
						infile=open(file,0);
						if ( infile<0 ) {
							infile=open(Ifile,0);
						}
					} else {
						lseek(infile, 0L, 0);
					}
					if ( infile<0 ) {
						infile=0;
						error(NOTOPEN);
					} else {
						if ( cntflg ) {
							var[9] = cntval;
						} else {
							var[9] = 1;
						}
					}
				} else {
					oclose();
					outfile=open(file,1);
					if ( outfile<0 ) {
						outfile=creat(file,0644);
#ifndef EDDT
					} else {
						lseek(outfile,0L,2);
#endif
					}
				}

			} else {
				if ( modif == '<' ) {
					iclose(-1, 0);
				} else {
					oclose();
				}
			}
			lp--;
		}
		break;
#if 0
	case 'p':
		if ( kernel == 0 ) {
			printf("not debugging kernel\n");
		} else {
			if ( adrflg ) {
				int pte = access(RD, dot, DSP, 0);
				masterpcbb = (pte&PG_PFNUM)*512;
			}
			getpcb();
		}
		break;
#endif
#endif	/* KERNEL */

	case 'd':
		if (adrflg) {
			if (adrval<2 || adrval>16) {
				printf("must have 2 <= radix <= 16");
				break;
			}
			printf("radix=%d base ten",radix=adrval);
		}
		break;

#ifdef	KERNEL
	case 'Q':
	case 'q':
		_exit(0);
		panic("KDB:CPU didn't reset\n");
		break;
#else	/* KERNEL */
	case 'q':
	case '%':
		done();
#endif	/* KERNEL */

	case 'w':
	case 'W':
		maxpos=(adrflg?adrval:MAXPOS);
		break;

	case 's':
	case 'S':
		maxoff=(adrflg?adrval:MAXOFF);
		break;

	case 'v':
	case 'V':
		prints("variables\n");
		for ( i=0;i<=35;i++ ) {
			    if ( var[i] ) {
				printc((i<=9 ? '0' : 'a'-10) + i);
				printf(" = %X\n",var[i]);
			}
		}
		break;

	case 'm':
	case 'M':
#if	MACH
		vm_map_print(dot);
#else	/* MACH */
		printmap("? map",&txtmap);
		printmap("/ map",&datmap);
#endif	/* MACH */
		break;

	case 0:
	case '?':
		if ( pid ) {
			printf("pcs id = %d\n",pid);
		} else {
			prints("no process\n");
		}
		sigprint();
		flushbuf();


	case 'p':
	case 'P':
		/* Spill the pmap */
		printf( "Dump of pmap, dirbase 0x%X:\n", get_dirbase() );
		pmap_dump_range( 0x2000, 0xFFFFF000 );
		return;

	case 'r':
	case 'R':
		printregs();
		return;
	case 'f':
		printfregs();
		return;
		
	case 'F':
		printpiperegs();
		return;

#ifndef KERNEL
	case 'c':
	case 'C':
		oldtrace(modif) ;
		break;

	case 'n':
	case 'N':
		newtrace(modif) ;
		break;
#endif

#ifdef	KERNEL
	case 'k':
	case 'c':
	case 'C':
		/*
		 *	Trace kernel stack only, to avoid crashing on
		 *	user stack page faults.
		 */
		if (adrflg == 0)
			adrval = db_ar0[FP];
		kdbtrace(adrval, db_ar0[PC]);
		break;

#ifdef HANS
	case 'K':
		/*
		 *	Trace specified thread's kernel stack.
		 */
		{
		    struct pcb *savecurpcb = curpcb;
		    int		saveadrflg = adrflg;

		    if (adrflg)
			curpcb = ((thread_t)adrval)->pcb;
		    adrflg = 0;
		    oldtrace('k');

		    adrflg = saveadrflg;
		    curpcb = savecurpcb;
		}
		break;
#endif
#endif	/* KERNEL */

		/*print externals*/
	case 'e':
	case 'E':
#ifdef DDD
		for (sp = symtab; sp < esymtab; sp++) {
			if (sp->n_type==(N_DATA|N_EXT) || sp->n_type==(N_BSS|N_EXT))
				printf("%s:%12t%R\n", sp->n_un.n_name, get(sp->n_value,DSP));
		}
#endif
		break;

	case 'a':
	case 'A':
		error("No algol 68 on NextDimension board.");
		/*NOTREACHED*/

		/*print breakpoints*/
#ifdef DDD
	case 'b':
		printf("bkpt\taddress\n");
		dr7 = _dr7();
		for (i = 0; i < 4; i++)
			if (dr7 & (2 << (i + i)))
				prbkpt(i);
		break;
#else
	case 'b':
#endif
	case 'B':
		printf("breakpoints\ncount%8tbkpt%24tcommand\n");
		for (bkptr=bkpthead; bkptr; bkptr=bkptr->nxtbkpt)
			if (bkptr->flag) {
				printf("%-8.8d",bkptr->count);
				psymoff(bkptr->loc,ISYM,"%24t");
				comptr=bkptr->comm;
				while ( *comptr ) {
					printc(*comptr++);
				}
			}
		break;
#ifndef i860
	case '<':
		printf("inb(%x) = %x\n", dot, inb(dot) & 0xff);
		break;
	case '>':
		if (expr(0) == 0)
			error("need value for outb");
		printf("outb(%x, %x) = %x\n", dot, expv, outb(dot, expv));
#endif
	default:
		printf( "%c: ", modif );
		error(BADMOD);
	}
}

static
prbkpt(n)
int n;
{
	register int pc;

#ifdef DDD
	switch (n) {

	case 0:
		pc = _dr0();
		break;
	case 1:
		pc = _dr1();
		break;
	case 2:
		pc = _dr2();
		break;
	case 3:
		pc = _dr3();
		break;

	}
#endif
	printf("   %d\t", n);
	psymoff(pc, ISYM, "\n");
	return;
}

printmap(s,amap)
string_t	s;
map *amap;
{
	int file;
	file=amap->ufd;
	printf("%s%12t`%s'\n",s,(file<0 ? "-" : (file==fcor ? corfil : symfil)));
	printf("b1 = %-16R",amap->b1);
	printf("e1 = %-16R",amap->e1);
	printf("f1 = %-16R",amap->f1);
	printf("\nb2 = %-16R",amap->b2);
	printf("e2 = %-16R",amap->e2);
	printf("f2 = %-16R",amap->f2);
	printc(EOR);
}

printregs()
{
	register REGPTR	p;
	long		v;
	int		i, n;

	i = 0;
	for ( p=reglist; p < &reglist[NREGS]; p++ ) {
		v = db_ar0[p->roffs];
		printf("%s", p->rname);
		n = dbstrlen(p->rname);
		while (n++ < 4)
			printc(' ');
		printf("%9X%  ", v);
#if	1
		if ((++i & 3) == 0)
			printc(EOR);
#else
		valpr(v,(p->roffs==PC?ISYM:DSYM));
		printc(EOR);
#endif
	}
	printc(EOR);
	printpc();
}

printfregs()
{
	register REGPTR	p;
	long		v;
	int		i, n;

	i = 0;
	while( i < NFREGS )
	{
		p = &freglist[i ^ 1];	/* Flip reg ordering for big-endian fetch */
		v = db_ar0[p->roffs];
		printf("%s", freglist[i].rname);
		n = dbstrlen(freglist[i].rname);
		while (n++ < 4)
			printc(' ');
		printf("%9X%  ", v);
#if	1
		if ((++i & 3) == 0)
			printc(EOR);
#else
		valpr(v,(p->roffs==PC?ISYM:DSYM));
		printc(EOR);
#endif
	}
	printc(EOR);
}
printpiperegs()
{
	register REGPTR	p;
	long		v;
	int		i, n;

	i = 0;
	while( i < NPIPEREGS )
	{
		p = &pipereglist[i];
		v = db_ar0[p->roffs];
		printf("%s", p->rname);
		n = dbstrlen(p->rname);
		while (n++ < 4)
			printc(' ');
		printf("%9X%  ", v);

		if ((++i & 3) == 0)
			printc(EOR);
	}
	printc(EOR);
}

getreg(regnam)
{
	register REGPTR	p;
	register string_t	regptr;
	char	*olp;
#if	CS_LINT
#else	/* CS_LINT */
	char		regnxt;
#endif	/* CS_LINT */

	olp=lp;
	for ( p=reglist; p < &reglist[NREGS]; p++ ) {
		regptr=p->rname;
		if ( (regnam == *regptr++) ) {
			while ( *regptr ) {
#if	CS_LINT
				if (readchar() != *regptr++) {
#else	/* CS_LINT */
				if ( (regnxt=readchar()) != *regptr++ ) {
#endif	/* CS_LINT */
					--regptr;
					break;
				}
			}
			if ( *regptr ) {
				lp=olp;
			} else {
#if	MACH
				int i = p->roffs;
#else	/* MACH */
				int i = kcore ? (int)p->rkern : p->roffs;
#endif	/* MACH */
#ifndef HANS
				i = (int) &db_ar0[i];
#endif
				return (i);
			}
		}
	}
	lp=olp;
	return(0);
}

printpc()
{
#ifdef	KERNEL
	dot = db_ar0[PC];
#else	/* KERNEL */
	dot= *(long *)(((long)&u)+PC);
#endif	/* KERNEL */
	psymoff(dot,ISYM,":%16t");
	printins(0,ISP,chkget(dot,ISP));
	printc(EOR);
}

char	*illinames[] = {
	"reserved addressing fault",
	"privileged instruction fault",
	"reserved operand fault"
};

char	*fpenames[] = {
	0,
	"integer overflow trap",
	"integer divide by zero trap",
	"floating overflow trap",
	"floating/decimal divide by zero trap",
	"floating underflow trap",
	"decimal overflow trap",
	"subscript out of range trap",
	"floating overflow fault",
	"floating divide by zero fault",
	"floating undeflow fault"
};

sigprint()
{
	if ( (signo>=0) && (signo<sizeof signals/sizeof signals[0]) ) {
		prints(signals[signo]);
	}
	switch (signo) {

	case SIGFPE:
		if ( (kdbsigcode > 0 &&
		    kdbsigcode < sizeof fpenames / sizeof fpenames[0]) ) {
			prints(" (");
			prints(fpenames[kdbsigcode]);
			prints(")");
		}
		break;

	case SIGILL:
		if ( (kdbsigcode >= 0 &&
		    kdbsigcode < sizeof illinames / sizeof illinames[0]) ) {
			prints(" (");
			prints(illinames[kdbsigcode]);
			prints(")");
		}
		break;
	}
}
