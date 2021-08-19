#include "defs.h"
#include "dis.h"


#define		BADADDR	-1L	/* used by the resynchronization	*/
				/* function to indicate that a restart	*/
				/* candidate has not been found		*/

int		space;		/* text/data/stack space */
long		loc;		/* location being dis-assembled */
unsigned short	curbyte;	/* byte at loc */
char		mneu[NLINE];	/* dis-assembled instruction */

/*
 *	compoff (lng, temp)
 *
 *	This routine will compute the location to which control is to be
 *	transferred.  'lng' is the number indicating the jump amount
 *	(already in proper form, meaning masked and negated if necessary)
 *	and 'temp' is a character array which already has the actual
 *	jump amount.  The result computed here will go at the end of 'temp'.
 *	(This is a great routine for people that don't like to compute in
 *	hex arithmetic.)
 */

compoff(lng, temp)
long	lng;
char	*temp;
{
	extern	long	loc;	/* from _extn.c */

	lng += loc;
	sprintf(temp,"%s <%lx>",temp,lng);
}
/*
 *	convnum (num, temp, flag)
 *
 *	Convert the passed number to hex
 *	leaving the result in the supplied string array.
 *	If  LEAD  is specified, precede the number with '0x' to
 *	indicate the base (used for information going to the mnemonic
 *	printout).  NOLEAD  will be used for all other printing (for
 *	printing the offset, object code, and the second byte in two
 *	byte immediates, displacements, etc.) and will assure that
 *	there are leading zeros.
 */

convnum(num,temp,flag)
unsigned	num;
char	temp[];
int	flag;

{

	if (flag == NOLEAD) 
		sprintf(temp,"%.4x",num);
	if (flag == LEAD)
		sprintf(temp,"0x%x",num);
	if (flag == NOLEADSHORT)
		sprintf(temp,"%.2x",num);
}

/*
 *	getbyte ()
 *
 *	read a byte, mask it, then return the result in 'curbyte'.
 *	The getting of all single bytes is done here.  The 'getbyte[s]'
 *	routines are the only place where the global variable 'loc'
 *	is incremented.
 */
int
getbyte()
{
	extern	int	space;
	extern	INT	dotinc;
	extern	long	dot;
	extern	unsigned short curbyte;
	char	temp[NCPS+1];
	char	byte;

	byte = chkget(dot + dotinc, space);
	dotinc++;
	curbyte = byte & 0377;
	convnum(curbyte, temp, NOLEADSHORT);
}


/*
 *	lookbyte ()
 *
 *	read a byte, mask it, then return the result in 'curbyte'.
 *	loc is incremented.
 */

int
lookbyte()
{
	extern	int	space;
	extern	INT	dotinc;
	extern	long	dot;
	extern	unsigned short curbyte;
	char	byte;

	byte = chkget(dot + dotinc, space);
	dotinc++;
	curbyte = byte & 0377;
}

/*
 *	printline ()
 *
 *	Print the disassembled line, consisting of the object code
 *	and the mnemonics.  The breakpointable line number, if any,
 *	has already been printed.
 */

printline()
{
	extern	char	mneu[];
	printf("%s",mneu); /* to print hex */
}

/*
 * scaled down version of sprintf.  Only handles %s and %x
 */
char *gcp;

sprintf(cp, fmt, a1, a2, a3, a4, a5)
char *cp, *fmt;
unsigned a1, a2, a3, a4, a5;
{
	unsigned args[5];

	args[0] = a1;  args[1] = a2;  args[2] = a3;
	args[3] = a4;  args[4] = a5;
	_sprintf(cp, fmt, args);
}

_sprintf(cp, fmt, ap)
char *cp, *fmt;
char **ap;
{
	char *str, c;
	long l;
	int prec, sign;

	while (c = *fmt)
		switch (*fmt++) {

		case '\\':
			switch (*fmt++) {

			case 'n':
				*cp++ = '\n';
				break;
			case '0':
				*cp++ = '\0';
				break;
			case 'b':
				*cp++ = '\b';
				break;
			case 't':
				*cp++ = '\t';
				break;
			default:
				cp++;

			}
			break;
		case '%':
			prec = 0;
			sign = 0;
again:
			switch ( *fmt++) {

			case '%':
				cp++;
				break;
			case 's':
				str = *ap++;
				while (*str) {
					prec--;
					*cp++ = *str++;
				}
				while (prec-- > 0)
					*cp++ = ' ';
				break;
			case 'd':
				l = *(long *)ap;
				ap++;
				gcp = cp;
				dec(l, sign);
				cp = gcp;
				break;
			case 'x':
				l = *(long *)ap;
				ap++;
				gcp = cp;
				hex(l, prec);
				cp = gcp;
				break;
			case '0':	case '1':	case '2':
			case '3':	case '4':	case '5':
			case '6':	case '7':	case '8':
			case '9':
				prec = (prec * 10) + (*(fmt - 1) - '0');
			case '+':
				sign = 1;
				goto again;
			case '-':
			case 'l':
			case '.':
				goto again;
			default:
				break;

			}
			break;
		default:
			*cp++ = c;
			break;

		}
	*cp = '\0';
	return;
}

hex(l, n)
unsigned long l;
int n;
{

	if (l > 15 || n > 1)
		hex(l >> 4, n - 1);
	*gcp++ = "0123456789abcdef"[l & 0xf];
	return;
}

dec(l, sign)
long l;
{
	char buf[20];
	int i = 0;

	if (l < 0) {
		*gcp++ = '-';
		l = -l;
	} else if (sign)
		*gcp++ = '+';
	do {
		buf[i++] = l%10 + '0';
		l /= 10;
	} while (l);
	while (i)
		*gcp++ = buf[--i];
}

/*
 * print out hex number, WITH SIGN
 */
prhex(l)
int l;
{
	char buf[10];

	if (l < 0) {
		l = -l;
		if (l < 0)
			sprintf(buf,"0/0");
		else
			sprintf(buf, "-0x%x", l);
	} else
		sprintf(buf, "0x%x", l);
	printf("%s", buf);
	return;
}

	char *
dbstrcpy(s1, s2)
char *s1, *s2;
{
	register char *cp;

	cp = s1;
	while (*cp++ = *s2++)
		;
	return(s1);
}

dbstrlen(s1)
char *s1;
{
	register int n;

	n = 0;
	while (*s1++)
		n++;
	return(n);
}

	char *
dbstrcat(s1, s2)
char *s1, *s2;
{
	register char *cp;

	cp = s1;
	cp += dbstrlen(s1);
	dbstrcpy(cp, s2);
	return(s1);
}

	char *
strchr(cp, c)
char *cp, c;
{

	while (*cp)
		if (*cp == c)
			return(cp);
		else
			cp++;
	return((char *)0);
}

strtol(sp, cp, base)
char *sp, **cp;
int base;
{
	register int n;
	char c;

	if (base == 0)
		base = 10;
	n = 0;
	if (*sp == '0') {
		sp++;
		base = 8;
		if (*sp == 'x') {
			sp++;
			base = 16;
		}
	}
	while ((*sp >= '0' && *sp <= '9') ||
	       (*sp >= 'a' && *sp <= 'f')) {
		if ((c = *sp++) >= 'a' && c <= 'f')
			c -= 'a' - '9' - 1;
		n = (n * base) + (c - '0');
	}
	if (cp)
		*cp = sp;
	return(n);
}

mkioptab()
{

	return;
}
