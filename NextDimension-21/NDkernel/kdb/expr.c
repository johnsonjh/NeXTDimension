/* 
 * Mach Operating System
 * Copyright (c) 1987 Carnegie-Mellon University
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 */
/*
 * HISTORY
 *  3-Oct-87  Michael Young (mwyoung) at Carnegie-Mellon University
 *	De-linted.
 *
 */
/*
 *
 *	UNIX debugger
 *
 */

#include "defs.h"

msg		BADSYM;
msg		BADVAR;
msg		BADKET;
msg		BADSYN;
msg		NOCFN;
msg		NOADR;
msg		BADLOC;

long		lastframe;
long		savlastf;
long		savframe;
long		savpc;
long		callpc;



char		*lp;
INT		radix;
string_t		errflg;
long		localval;
char		isymbol[1024];

char		lastc,peekc;

long		dot;
long		ditto;
INT		dotinc;
long		var[];
long		expv;

#ifdef i860

#undef isalpha
#undef isdigit
#undef isxdigit

isalpha(c)
register char c;
{
	if (c >= 'a' && c <= 'z')
		return(1);
	if (c >= 'A' && c <= 'Z')
		return(1);
	if (c == '_')
		return(1);
	return(0);
}

isupper(c)
register char c;
{

	return(c >= 'A' && c <= 'Z');
}

isdigit(c)
register char c;
{

	return (c >= '0' && c <= '9');
}

isxdigit(c)
register char c;
{

	return ((c >= 'a' && c <= 'f') ? 1 : isdigit(c));
}

#endif

expr(a)
{	/* term | term dyadic expr |  */
	INT		rc;
	long		lhs;

	rdc();
	lp--;
	rc=term(a);

	while ( rc ) {
		lhs = expv;

		switch ((int)readchar()) {

		case '+':
			term(a|1);
			expv += lhs;
			break;

		case '-':
			term(a|1);
			expv = lhs - expv;
			break;

		case '#':
			term(a|1);
			expv = round(lhs,expv);
			break;

		case '*':
			term(a|1);
			expv *= lhs;
			break;

		case '%':
			term(a|1);
			expv = lhs/expv;
			break;

		case '&':
			term(a|1);
			expv &= lhs;
			break;

		case '|':
			term(a|1);
			expv |= lhs;
			break;

		case ')':
			if ( (a&2)==0 ) {
				error(BADKET);
			}

		default:
			lp--;
			return(rc);
		}
	}
	return(rc);
}

term(a)
{	/* item | monadic item | (expr) | */

	switch ((int)readchar()) {

	case '*':
		term(a|1);
		expv=chkget(expv,DSP);
		return(1);

	case '@':
		term(a|1);
		expv=chkget(expv,ISP);
		return(1);

	case '-':
		term(a|1);
		expv = -expv;
		return(1);

	case '~':
		term(a|1);
		expv = ~expv;
		return(1);

	case '#':
		term(a|1);
		expv = !expv;
		return(1);

	case '(':
		expr(2);
		if ( *lp!=')' ) {
			error(BADSYN);
		} else {
			lp++;
			return(1);
		}

	default:
		lp--;
		return(item(a));
	}
}

item(a)
{	/* name [ . local ] | number | . | ^ | <var | <register | 'x | | */
	INT		base, d;
	char		savc;
#if	CS_LINT
#else	/* CS_LINT */
	BOOL		hex;
#endif	/* CS_LINT */
	long		frame;
	register struct nlist *symp;
	int regptr;

#if	CS_LINT
#else	/* CS_LINT */
	hex=FALSE;
#endif	/* CS_LINT */

	readchar();
	if ( symchar(0) ) {
		readsym();
		if ( (symp=lookup(isymbol))==0 ) {
			error(BADSYM);
		} else {
			expv = symp->n_value;
		}
		lp--;

	} else if ( getnum(readchar) ) {
		;
/* .varname */
	} else if ( lastc=='.' ) {
		readchar();
		if ( symchar(0) ) {
			lastframe=savlastf;
			callpc=savpc;
			chkloc(savframe);
		} else {
			expv=dot;
		}
		lp--;

	} else if ( lastc=='"' ) {
		expv=ditto;

	} else if ( lastc=='+' ) {
		expv=inkdot(dotinc);

	} else if ( lastc=='^' ) {
		expv=inkdot(-dotinc);

	} else if ( lastc=='<' ) {
		savc=rdc();
		if ( regptr=getreg(savc) ) {
#ifdef	KERNEL
			expv = *(int *)regptr;
#else	/* KERNEL */
			if ( kcore ) {
				expv = *(int *)regptr;
			} else {
				expv= * (long *)(((long)&u)+regptr);
			}
#endif	/* KERNEL */
		} else if ( (base=varchk(savc)) != -1 ) {
			expv=var[base];
		} else {
			error(BADVAR);
		}

	} else if ( lastc=='\'' ) {
		d=4;
		expv=0;
		while ( quotchar() ) {
			if ( d-- ) {
				expv = (expv << 8) | lastc;
			} else {
				error(BADSYN);
			}
		}

	} else if ( a ) {
		error(NOADR);
	} else {
		lp--;
		return(0);
	}
	return(1);
}

/* service routines for expression reading */
getnum(rdf) int (*rdf)();
{
	INT base,d,frpt;
	BOOL hex;
	union{
#ifdef FLOAT
		float r;
#endif
		long i;
	}
	real;
	if ( isdigit(lastc) || (hex=TRUE, lastc=='#' && isxdigit((*rdf)())) ) {
		expv = 0;
		base = (hex ? 16 : radix);
		while ( (base>10 ? isxdigit(lastc) : isdigit(lastc)) ) {
			expv = (base==16 ? expv<<4 : expv*base);
			if ( (d=convdig(lastc))>=base ) {
				error(BADSYN);
			}
			expv += d;
			(*rdf)();
			if ( expv==0 ) {
				if ( (lastc=='x' || lastc=='X') ) {
					hex=TRUE;
					base=16;
					(*rdf)();
				} else if ( (lastc=='t' || lastc=='T') ) {
					hex=FALSE;
					base=10;
					(*rdf)();
				} else if ( (lastc=='o' || lastc=='O') ) {
					hex=FALSE;
					base=8;
					(*rdf)();
				}
			}
		}
#ifdef FLOAT
		if ( lastc=='.' && (base==10 || expv==0) && !hex ) {
			real.r=expv;
			frpt=0;
			base=10;
			while ( isdigit((*rdf)()) ) {
				real.r *= base;
				frpt++;
				real.r += lastc-'0';
			}
			while ( frpt-- ) {
				real.r /= base;
			}
			expv = real.i;
		}
#endif
		peekc=lastc;
		/*		lp--; */
		return(1);
	} else {
		return(0);
	}
}

readsym()
{
	register char	*p;

	p = isymbol;
	do {
		if ( p < &isymbol[sizeof(isymbol)-1] ) {
			*p++ = lastc;
		}
		readchar();
	}
	while( symchar(1) ) ;
	*p++ = 0;
}

convdig(c)
char c;
{
	if ( isdigit(c) ) {
		return(c-'0');
	} else if ( isxdigit(c) ) {
		return(c-'a'+10);
	} else {
		return(17);
	}
}

symchar(dig)
{
	if ( lastc=='\\' ) {
		readchar();
		return(TRUE);
	}
	return( isalpha(lastc) || lastc=='_' || dig && isdigit(lastc) );
}

varchk(name)
{
	if ( isdigit(name) ) {
		return(name-'0');
	}
	if ( isalpha(name) ) {
		return((name&037)-1+10);
	}
	return(-1);
}

chkloc(frame)
long		frame;
{
	readsym();
	do {
		if ( localsym(frame)==0 ) {
			error(BADLOC);
		}
		expv=localval;
	}
#ifdef DDD
	while( !eqsym(cursym->n_un.n_name,isymbol,'~') ) ;
#else
	while( !eqsym(cursym->n_name,isymbol,'~') ) ;
#endif
}

eqsym(s1, s2, c)
register char *s1, *s2;
{

	if (!strcmp(s1,s2))
		return (1);
	if (*s1 == c && !strcmp(s1+1, s2))
		return (1);
	return (0);
}
