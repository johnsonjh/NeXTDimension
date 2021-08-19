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
/*
 *
 *	UNIX debugger
 *
 */

#include "defs.h"

msg		BADEQ;
msg		NOMATCH;
msg		BADVAR;
msg		BADCOM;

map		txtmap;
map		datmap;
INT		executing;
char		*lp;
INT		fcor;
INT		fsym;
INT		mkfault;
string_t		errflg;

char		lastc;
char		eqformat[512] = "z";
char		stformat[512] = "X\"= \"^i";

long		dot;
long		ditto;
INT		dotinc;
INT		lastcom = '=';
long		var[];
long		locval;
long		locmsk;
INT		pid;
long		expv;
long		adrval;
INT		adrflg;
long		cntval;
INT		cntflg;

extern int *db_ar0;



/* command decoding */

command(buf,defcom)
string_t		buf;
char		defcom;
{
#if	CMU
	INT		itype, ptype, modifier;
	long		regptr;
#else	/* CMU */
	INT		itype, ptype, modifier, regptr;
#endif	/* CMU */
	BOOL		longpr, eqcom;
	char		wformat[1];
	char		savc;
	long		w, savdot;
	string_t		savlp=lp;

	if ( buf ) {
		if ( *buf==EOR ) {
			return(FALSE);
		} else {
			lp=buf;
		}
	}

	do {
		if ( adrflg=expr(0) ) {
			dot=expv;
			ditto=dot;
		}
		adrval=dot;
		if ( rdc()==',' && expr(0) ) {
			cntflg=TRUE;
			cntval=expv;
		} else {
			cntflg=FALSE;
			cntval=1;
			lp--;
		}

		if ( !eol(rdc()) ) {
			lastcom=lastc;
		} else {
			if ( adrflg==0 ) {
				dot=inkdot(dotinc);
			}
			lp--;
			lastcom=defcom;
		}

		switch(lastcom&0177) {

		case '/':
			itype=DSP;
			ptype=DSYM;
			goto trystar;

		case '=':
			itype=NSP;
			ptype=0;
			goto trypr;

		case '?':
			itype=ISP;
			ptype=ISYM;
			goto trystar;

trystar:
			if ( rdc()=='*' ) {
				lastcom |= QUOTE;
			} else {
				lp--;
			}
			if ( lastcom&QUOTE ) {
				itype |= STAR;
				ptype = (DSYM+ISYM)-ptype;
			}

trypr:
			longpr=FALSE;
			eqcom=lastcom=='=';
			switch (rdc()) {

			case 'm':
				{/*reset map data*/
					INT		fcount;
					map		*smap;
					union{
						map *m;
						long *mp;
					}
					amap;

					if ( eqcom ) {
						error(BADEQ);
					}
					smap=(itype&DSP?&datmap:&txtmap);
					amap.m=smap;
					fcount=3;
					if ( itype&STAR ) {
						amap.mp += 3;
					}
					while ( fcount-- && expr(0) ) {
						*(amap.mp)++ = expv;
					}
					if ( rdc()=='?' ) {
						smap->ufd=fsym;
					} else if ( lastc == '/' ) {
						smap->ufd=fcor;
					} else {
						lp--;
					}
				}
				break;

			case 'L':
				longpr=TRUE;
			case 'l':
				/*search for exp*/
				if ( eqcom ) {
					error(BADEQ);
				}
				dotinc=(longpr?4:2);
				savdot=dot;
				expr(1);
				locval=expv;
				if ( expr(0) ) {
					locmsk=expv;
				} else {
					locmsk = -1L;
				}
				if ( !longpr ) {
					locmsk &= 0xFFFF;
					locval &= 0xFFFF;
				}
				for (;;) {
					w=get(dot,itype);
					if ( errflg || mkfault || (w&locmsk)==locval ) {
						break;
					}
					dot=inkdot(dotinc);
				}
				if ( errflg ) {
					dot=savdot;
					errflg=NOMATCH;
				}
				psymoff(dot,ptype,"");
				break;

			case 'W':
				longpr=TRUE;
			case 'w':
				if ( eqcom ) {
					error(BADEQ);
				}
				wformat[0]=lastc;
				expr(1);
				do {
					savdot=dot;
					psymoff(dot,ptype,":%16t");
					exform(1,wformat,itype,ptype);
					errflg=0;
					dot=savdot;
					if ( longpr ) {
						put(dot,itype,expv);
					} else {
						put(dot,itype,itol(get(dot+2,itype),expv));
					}
					savdot=dot;
					printf("=%8t");
					exform(1,wformat,itype,ptype);
					newline();
				}
				while(  expr(0) && errflg==0 ) ;
				dot=savdot;
				chkerr();
				break;

			default:
				lp--;
				getformat(eqcom ? eqformat : stformat);
				if ( !eqcom ) {
					psymoff(dot,ptype,":%16t");
				}
				scanform(cntval,(eqcom?eqformat:stformat),itype,ptype);
			}
			break;

		case '>':
			lastcom=0;
			savc=rdc();
			if ( regptr=getreg(savc) ) {
				if ( kcore ) {
					*(int *)regptr = dot;
				} else {
					* (long *) (((long)db_ar0)+regptr)=dot;
#ifdef	KERNEL
#else	/* KERNEL */
					ptrace(WUREGS,pid,regptr,* (long *) (((long)&u)+regptr));
#endif	/* KERNEL */
				}
			} else if ( (modifier=varchk(savc)) != -1 ) {
				var[modifier]=dot;
			} else {
				error(BADVAR);
			}
			break;

		case '!':
			lastcom=0;
			PageTrace(nextchar());
			break;

		case '$':
			lastcom=0;
			printtrace(nextchar());
			break;

		case ':':
			if ( !executing ) {
				executing=TRUE;
				subpcs(nextchar());
				executing=FALSE;
				lastcom=0;
			}
			break;

		case 0:
			prints(DBNAME);
			break;

		default:
			error(BADCOM);
		}

		flushbuf();
	}
	while( rdc()==';' ) ;
	if ( buf ) {
		lp=savlp;
	} else {
		lp--;
	}
	return(adrflg && dot!=0);
}

