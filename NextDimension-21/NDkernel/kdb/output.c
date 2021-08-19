/* 
 * Mach Operating System
 * Copyright (c) 1987 Carnegie-Mellon University
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 */
/*
 *
 *	UNIX debugger
 *
 */

#include "defs.h"
/*
 **************************************************************************
 * HISTORY
 *  3-Oct-87  Michael Young (mwyoung) at Carnegie-Mellon University
 *	De-linted.
 *
 * 11-Oct-85  Robert V Baron (rvb) at Carnegie-Mellon University
 *	FLushed the stupid #
 *
 **************************************************************************
 */

INT		mkfault;
INT		infile;
INT		outfile = 1;
long		maxpos;
long		maxoff;
INT		radix = 16;

char		printbuf[MAXLIN];
char		*printptr = printbuf;
char		*digitptr;
msg		TOODEEP;

static int _printf(string_t,long *);

eqstr(s1, s2)
register string_t	s1, s2;
{
	register string_t	 es1;

	while ( *s1++ == *s2 ) {
		if ( *s2++ == 0 ) {
			return(1);
		}
	}
	return(0);
}

length(s)
register string_t		s;
{
	INT		n = 0;
	while ( *s++ ) {
		n++;
	}
	return(n);
}

printc(c)
char		c;
{
	char		d;
	string_t		q;
	INT		posn, tabs, p;

	if ( mkfault ) {
		return;
	} else if ( (*printptr=c)==EOR ) {
		tabs=0;
		posn=0;
		q=printbuf;
		for ( p=0; p<printptr-printbuf; p++ ) {
			    d=printbuf[p];
			if ( (p&7)==0 && posn ) {
				tabs++;
				posn=0;
			}
			if ( d==' ' ) {
				posn++;
			} else {
				while ( tabs>0 ) {
					*q++='	';
					tabs--;
				}
				while ( posn>0 ) {
					*q++=' ';
					posn--;
				}
				*q++=d;
			}
		}
		*q++=EOR;
#ifdef EDDT
		printptr=printbuf;
		do putchar(*printptr++);
		while (printptr<q);
#else
		write(outfile,printbuf,q-printbuf);
#endif
		printptr=printbuf;
	} else if ( c=='	' ) {
		*printptr++=' ';
		while ( (printptr-printbuf)&7 ) {
			*printptr++=' ';
		}
	} else if ( c ) {
		printptr++;
	}
	if ( printptr >= &printbuf[MAXLIN-9] ) {
		write(outfile, printbuf, printptr - printbuf);
		printptr = printbuf;
	}
}

charpos()
{
	return(printptr-printbuf);
}

flushbuf()
{
	if ( printptr!=printbuf ) {
		printc(EOR);
	}
}

/*VARARGS1*/
printf(fmat, a1, a2, a3, a4, a5, a6)
string_t		fmat;
long a1, a2, a3, a4, a5, a6;
{
	long args[6];

	args[0] = a1;  args[1] = a2; args[2] = a3;
	args[3] = a4;  args[4] = a5; args[5] = a6;
	_printf(fmat, args);
}

static
_printf(fmat,dptr)
string_t		fmat;
long			*dptr;
{
	string_t		fptr, s;
	INT		*vptr;
#ifndef	KERNEL
	double		*rptr;
#endif	/* KERNEL */
	INT		width, prec;
	char		c, adj;
#ifdef	KERNEL
	INT		x, n;
#else	/* KERNEL */
	INT		x, decpt, n;
#endif	/* KERNEL */
	long		lx;
	char		digits[64];

	fptr = fmat;
	vptr = (INT *)dptr;

	while ( c = *fptr++ ) {
		if ( c!='%' ) {
			printc(c);
		} else {
			if ( *fptr=='-' ) {
				adj='l';
				fptr++;
			} else {
				adj='r';
			}
			width=convert(&fptr);
			if ( *fptr=='.' ) {
				fptr++;
				prec=convert(&fptr);
			} else {
				prec = -1;
			}
			digitptr=digits;
#if	CMU
#ifndef	KERNEL
			rptr=(double *)dptr;
#endif	/* KERNEL */
			x = shorten(lx = *dptr++);
#else	/* CMU */
#ifndef	KERNEL
			rptr=dptr;
#endif	/* KERNEL */
			x = shorten(lx = *dptr++);
#endif	/* CMU */
			s=0;
			switch (c = *fptr++) {

			case 'd':
			case 'u':
				printnum(x,c,10);
				break;
			case 'o':
				printoct(itol(0,x),0);
				break;
			case 'q':
				lx=x;
				printoct(lx,-1);
				break;
			case 'x':
				printdbl(itol(0,x),c,16);
				break;
			case 'r':
				printdbl(lx=x,c,radix);
				break;
			case 'R':
				printdbl(lx,c,radix);
				vptr++;
				break;
			case 'Y':
				printdate(lx);
				vptr++;
				break;
			case 'D':
			case 'U':
				printdbl(lx,c,10);
				vptr++;
				break;
			case 'O':
				printoct(lx,0);
				vptr++;
				break;
			case 'Q':
				printoct(lx,-1);
				vptr++;
				break;
			case 'X':
				printdbl(lx,'x',16);
				vptr++;
				break;
			case 'c':
				printc(x);
				break;
			case 's':
#if	CMU
				s=(char *)lx;
				break;
#else	/* CMU */
				s=lx;
				break;
#endif	/* CMU */
#ifdef	KERNEL
#else	/* KERNEL */
#ifndef EDDT
			case 'f':
			case 'F':
#ifdef	VAX
				dptr++;
				sprintf(s=digits,"%+.16e",*rptr,*(rptr+4));
				prec= -1;
				break;
#else
				vptr += 7;
				s=ecvt(*rptr, prec, &decpt, &n);
				*digitptr++=(n?'-':'+');
				*digitptr++ = (decpt<=0 ? '0' : *s++);
				if ( decpt>0 ) {
					decpt--;
				}
				*digitptr++ = '.';
				while ( *s && prec-- ) {
					*digitptr++ = *s++;
				}
				while ( *--digitptr=='0' ) ;
				digitptr += (digitptr-digits>=3 ? 1 : 2);
				if ( decpt ) {
					*digitptr++ = 'e';
					printnum(decpt,'d',10);
				}
				s=0;
				prec = -1;
				break;
#endif
#endif
#endif	/* KERNEL */
			case 'm':
				vptr--;
				break;
			case 'M':
				width=x;
				break;
			case 'T':
			case 't':
				if ( c=='T' ) {
					width=x;
				} else {
					dptr--;
				}
				if ( width ) {
					width -= charpos()%width;
				}
				break;
			default:
				printc(c);
				dptr--;
			}

			if ( s==0 ) {
				*digitptr=0;
				s=digits;
			}
			n=length(s);
			n=(prec<n && prec>=0 ? prec : n);
			width -= n;
			if ( adj=='r' ) {
				while ( width-- > 0 ) {
					printc(' ');
				}
			}
			while ( n-- ) {
				printc(*s++);
			}
			while ( width-- > 0 ) {
				printc(' ');
			}
			digitptr=digits;
		}
	}
}

printdate(tvec)
long		tvec;
{
	register INT		i;
	register string_t	timeptr;

#if !defined(EDDT) && !defined(KERNEL)
	timeptr = ctime(&tvec);
#else /* !defined(EDDT) && !defined(KERNEL) */
	timeptr="????????????????????????";
#ifdef	lint
	printdate(tvec);	/* reference tvec */
#endif	/* lint */
#endif /* !defined(EDDT) && !defined(KERNEL) */
	for ( i=20; i<24; i++ ) {
		*digitptr++ = *(timeptr+i);
	}
	for ( i=3; i<19; i++ ) {
		*digitptr++ = *(timeptr+i);
	}
} /*printdate*/

prints(s)
char *s;
{
	printf("%s",s);
}

newline()
{
	printc(EOR);
}

convert(cp)
register string_t	*cp;
{
	register char	c;
	INT		n;
	n=0;
	while ( ((c = *(*cp)++)>='0') && (c<='9') ) {
		n=n*10+c-'0';
	}
	(*cp)--;
	return(n);
}

printnum(n,fmat,base)
register INT		n;
{
	register char	k;
	register INT		*dptr;
	INT		digs[15];
	dptr=digs;
	if ( n<0 && fmat=='d' ) {
		n = -n;
		*digitptr++ = '-';
	}
	n &= 0xffff;
	while ( n ) {
		*dptr++ = ((unsigned)(n&0xffff))%base;
		n=((unsigned)(n&0xffff))/base;
	}
	if ( dptr==digs ) {
		*dptr++=0;
	}
	while ( dptr!=digs ) {
		k = *--dptr;
		*digitptr++ = (k+(k<=9 ? '0' : 'a'-10));
	}
}

printoct(o,s)
long		o;
INT		s;
{
	INT		i;
	long		po = o;
	char		digs[12];

	if ( s ) {
		if ( po<0 ) {
			po = -po;
			*digitptr++='-';
		} else {
			if ( s>0 ) {
				*digitptr++='+';
			}
		}
	}
	for ( i=0;i<=11;i++ ) {
		    digs[i] = po&7;
		po >>= 3;
	}
	digs[10] &= 03;
	digs[11]=0;
	for ( i=11;i>=0;i-- ) {
		    if ( digs[i] ) {
			break;
		}
	}
	for ( i++;i>=0;i-- ) {
		    *digitptr++=digs[i]+'0';
	}
}

#ifdef BROKEN_CROCK
printdbl(lxy,fmat,base)
long lxy;
char fmat;
int base;
{
	int digs[20];
	int *dptr;
	char k;
#ifndef MULD2
	register char *cp1;
	cp1=digs;
	if ((lxy&0xFFFF0000L)==0xFFFF0000L) {
		*cp1++='-';
		lxy= -lxy;
	}
	sprintf(cp1,base==16 ? "%X" : "%D",lxy);
	cp1=digs;
	while (*digitptr++= *cp1++)
	    ;
	--digitptr;
#else
#ifdef FLOAT
	double f ,g;
#else
	unsigned long f, g;
#endif
	unsigned long q;
#ifdef i860
	INT lx,ly;
	ly=lxy;
	lx=(lxy>>16)&0xFFFF;
#endif
	dptr=digs;
	if ( fmat=='D' || fmat=='r' ) {
		f=itol(lx,ly);
		if ( f<0 ) {
			*digitptr++='-';
			f = -f;
		}
	} else {
		if ( lx==-1 ) {
			*digitptr++='-';
			f=leng(-ly);
		} else {
			f=leng(lx);
			f *= itol(1,0);
			f += leng(ly);
		}
#if	! KERNEL
		if ( fmat=='x' ) {
			*digitptr++='#';
		}
#endif	/* ! KERNEL */
	}
	while ( f ) {
		q=f/base;
		g=q;
		*dptr++ = f-g*base;
		f=q;
	}
	if ( dptr==digs || dptr[-1]>9 ) {
		*dptr++=0;
	}
	while ( dptr!=digs ) {
		k = *--dptr;
		*digitptr++ = (k+(k<=9 ? '0' : 'a'-10));
	}
#endif
}
#else
printdbl(lxy,fmat,base)
long lxy;
char fmat;
int base;
{
	int digs[20];
	int *dptr;
	char k;
	unsigned long f,g,q;
	
	dptr = digs;
	
	if ( fmat == 'D' || fmat == 'r' )
	{
		if ( lxy < 0 )
		{
			*digitptr++='-';
			lxy = -lxy;
		}
	}
	f = lxy;
	while ( f ) {
		q=f/base;
		g=q;
		*dptr++ = f-g*base;
		f=q;
	}
	if ( dptr==digs || dptr[-1]>9 ) {
		*dptr++=0;
	}
	while ( dptr!=digs ) {
		k = *--dptr;
		*digitptr++ = (k+(k<=9 ? '0' : 'a'-10));
	}

}
#endif
			
#define	MAXIFD	5
struct {
	int	fd;
	int	r9;
}

istack[MAXIFD];
int	ifiledepth;

#ifdef	KERNEL
iclose(){}
oclose(){}
#else	/* KERNEL */
iclose(stack, err)
{
	if ( err ) {
		if ( infile ) {
			close(infile);
			infile=0;
		}
		while ( --ifiledepth >= 0 ) {
			if ( istack[ifiledepth].fd ) {
				close(istack[ifiledepth].fd);
			}
		}
		ifiledepth = 0;
	} else if ( stack == 0 ) {
		if ( infile ) {
			close(infile);
			infile=0;
		}
	} else if ( stack > 0 ) {
		if ( ifiledepth >= MAXIFD ) {
			error(TOODEEP);
		}
		istack[ifiledepth].fd = infile;
		istack[ifiledepth].r9 = var[9];
		ifiledepth++;
		infile = 0;
	} else {
		if ( infile ) {
			close(infile);
			infile=0;
		}
		if ( ifiledepth > 0 ) {
			infile = istack[--ifiledepth].fd;
			var[9] = istack[ifiledepth].r9;
		}
	}
}

oclose()
{
	if ( outfile!=1 ) {
		flushbuf();
		close(outfile);
		outfile=1;
	}
}
#endif	/* KERNEL */

endline()
{

	if (maxpos <= charpos())
		printf("\n");
}
