
#include	"defs.h"
#include	"dis.h"

#define		OPLEN	35	/* maximum length of a single operand	*/
				/* (will be used for printing)		*/

char	operand[3][OPLEN];	/* to store operands as they	*/
				/* are encountered		*/
char mnemonic[OPLEN];		/* op-code part */

#define FOPC(x)	((x)&0x7f)
#define MKDISP(h,l)	((short)(((h)<<11)|(l)))
#define MKUNS(h,l)	(((h)<<11)|(l))

static long hexscan(char *);
static int putop(char *);
static int abrev(int);
static int dis_dot(long,int,unsigned long);
static get_ins(unsigned*,unsigned*,unsigned*,unsigned*,unsigned*,unsigned*,unsigned);
static int mkd26(unsigned,unsigned,unsigned,unsigned);

/*
 *	db_dis(addr, count)
 *
 *	disassemble `count' instructions starting at `addr'
 */

printins(fmt, Idsp, ins)
char fmt;
int Idsp;
unsigned ins;
{
	extern unsigned short curbyte;
	extern int space;
	extern INT dotinc;
	extern long dot;
	int n;

	space = Idsp;
	dotinc = 4;		/* All insns are 4 bytes. */
	curbyte = ins;
	n = dis_dot(dot, Idsp, ins);
	n = abrev(n);
	printf("%s\t", mnemonic);
	switch (n) {
	case 1:
		putop(operand[0]);
		break;
	case 2:
		putop(operand[0]);
		printc(',');
		putop(operand[1]);
		break;
	case 3:
		putop(operand[0]);
		printc(',');
		putop(operand[1]);
		printc(',');
		putop(operand[2]);
		break;
	}
	return;
}

static
long
hexscan(s)
char *s;
{
	long l = 0;
	char c;

	while (c = *s++) {
		if (c >= '0' && c <= '9')
			l = l<<4 | (c-'0');
		else if (c >= 'a' && c <= 'f')
			l = l<<4 | (c-'a'+10);
		else
			return l;
	}
	return l;
}

static
putop(s)
char *s;
{
	char *t, *strchr();
	long l;
	extern int space;

	printf("%s", s);
	t = strchr(s, '<');
	if (t) {
		l = hexscan(t+1);
		printf(" [");
		psymoff(l, space, "]");
	}
}

static
int
abrev(n)
{
	extern	char 	*dbstrcpy();

	if (n == 3 && strcmp(mnemonic,"shl") == 0 &&
	    strcmp(operand[0], "r0") == 0) {
		if (strcmp(operand[2], "r0") == 0) {
			dbstrcpy(mnemonic, "nop");
			return 0;
		} else {
			dbstrcpy(mnemonic, "mov");
			dbstrcpy(operand[0], operand[1]);
			dbstrcpy(operand[1], operand[2]);
			return 2;
		}
	}
	return n;
}

/*
 *	dis_dot ()
 *
 *	disassemble a text section
 */

static
dis_dot(ldot,idsp,insn)
long ldot;
int idsp;
unsigned long insn;
{
	/* the following arrays are contained in tables.c	*/
	extern	struct	instable	distable[8][8];

	extern	char	*IREG[32];
	extern	char	*FREG[32];
	extern	char	*CONTROLREG[8];

	/* the following entries are from _extn.c	*/
	extern	long	 loc;
	extern	unsigned short curbyte;

	/* the following routines are in _utls.c	*/
	extern	void	printline();
	extern	void	looklabel();
	extern	void	compoff();
	extern	void	convert();
	extern	short	sect_name();
	extern	void	getbyte();
	extern	void	lookbyte();

	/* libc */
	extern	void	exit();
	extern	char 	*dbstrcat();
	extern	char 	*dbstrcpy();

	struct instable *dp;

	/* parts of the opcode */
	unsigned oct1,oct2, src2,dest,src1, off11;
	long lngval;

	char	temp[NCPS+1];
	char *ainc;

	loc = ldot;

	/*
	 * An instruction is disassembled with each iteration of the
	 * following loop.  The loop is terminated upon completion of the
	 * section (loc minus the section's physical address becomes equal
	 * to the section size) or if the number of bad op codes encountered
	 * would indicate this disassembly is hopeless.
	 */

	mnemonic[0] = '\0';
	operand[0][0] = '\0';
	operand[1][0] = '\0';
	operand[2][0] = '\0';

	get_ins(&oct1,&oct2, &src2,&dest,&src1, &off11, insn);
	dp = &distable[oct1][oct2];

	if (dp->indirect != TERM) {
		dp = dp->indirect + FOPC(off11);
	}

	if (dp->indirect != TERM || dp->name == NULL) {
		sprintf(mnemonic,"***** Error - bad opcode\n");
		return 0;
	}

	/* print the mnemonic */
	if (dp->suffix == FLTS) {
		if (off11 & FLT_D)
			dbstrcat(mnemonic, "d.");
		if (off11 & FLT_P)
			dbstrcat(mnemonic, "p.");
	}
	(void) dbstrcat(mnemonic,dp -> name);  /* print the mnemonic */
	switch(dp->suffix) {
	case FLTS:
		dbstrcat(mnemonic, (off11&FLT_SD) ? ".d" : ".s");
		dbstrcat(mnemonic, (off11&FLT_RD) ? "d" : "s");
		break;
	case WL1:
		(void) dbstrcat(mnemonic, ((off11&1)? ".l" : ".s") );
		off11 &= ~1;
		break;
	case LDQ_A:
		dbstrcat(mnemonic, (off11&2) ? ".l" :
			(off11&4) ? ".q" : ".d");
		/* fall through */
	case M3:
		ainc = (off11&1) ? "++" : "";
		off11 &= ~((off11&2) ? 3 : 7);
		break;
	}

	/*
	 * Each instruction has a particular instruction syntax format
	 * stored in the disassembly tables.  The assignment of formats
	 * to instructins was made by the author.  Individual formats
	 * are explained as they are encountered in the following
	 * switch construct.
	 */

	switch(dp -> adr_mode){
	case B26:	/* branch-[src2:dest:src1:offset] */
		lngval = mkd26(src2,dest,src1,off11);
		lngval *= 4;
		sprintf(operand[0], "%+d", lngval);
		compoff(lngval+4, operand[0]);
		return 1;
	case rS12B:	/* ireg-src1,ireg-src2,branch-[dest:offset] */
		dbstrcpy(operand[0], IREG[src1]);
		goto rsoff;
	case iS12B:	/* const-src1,ireg-src2,branch-[dest:offset] */
		sprintf(operand[0], "%d", src1);
rsoff:
		dbstrcpy(operand[1], IREG[src2]);
		lngval = MKDISP(dest,off11);
		lngval *= 4;
		sprintf(operand[2], "%+d", lngval);
		compoff(lngval+4, operand[2]);
		return 3;
	case S1:	/* ireg-src1 */
		dbstrcpy(operand[0], IREG[src1]);
		return 1;
	case uS12D:	/* uconst-[src1:offset],ireg-src2,ireg-dest */
		sprintf(operand[0], "0x%x", MKUNS(src1,off11));
		goto didone;
	case iS12D:	/* const-[src1:offset],ireg-src2,ireg-dest */
		sprintf(operand[0], "%d", MKDISP(src1,off11));
		goto didone;
	case rS12D:	/* ireg-src1,ireg-src2,ireg-dest */
		dbstrcpy(operand[0], IREG[src1]);
didone:
		dbstrcpy(operand[1], IREG[src2]);
		dbstrcpy(operand[2], IREG[dest]);
		return 3;
	case fS12D:	/* freg-src1,freg-src2,freg-dest */
		dbstrcpy(operand[0], FREG[src1]);
		dbstrcpy(operand[1], FREG[src2]);
		dbstrcpy(operand[2], FREG[dest]);
		return 3;
	case fS1D:	/* freg-src1,freg-dest */
		dbstrcpy(operand[0], FREG[src1]);
		dbstrcpy(operand[1], FREG[dest]);
		return 2;
	case fS1Dr:	/* freg-src1,ireg-dest */
		dbstrcpy(operand[0], FREG[src1]);
		dbstrcpy(operand[1], IREG[dest]);
		return 2;
	case fS2D:	/* freg-src2,freg-dest */
		dbstrcpy(operand[0], FREG[src2]);
		dbstrcpy(operand[1], FREG[dest]);
		return 2;
	case C_R:	/* creg-src2,ireg-dest */
		dbstrcpy(operand[0], CONTROLREG[src2]);
		dbstrcpy(operand[1], IREG[dest]);
		return 2;
	case R_C:	/* ireg-src1,creg-src2 */
		dbstrcpy(operand[0], IREG[src1]);
		dbstrcpy(operand[1], CONTROLREG[src2]);
		return 2;
	case rA_F:	/* ireg-src1(ireg-src2)optainc,freg-dest */
		sprintf(operand[0], "%s(%s)%s", IREG[src1],IREG[src2],ainc);
		goto didf0;
	case iA_F:	/* const-[src1:offset](ireg-src2)optainc,freg-dest */
		sprintf(operand[0], "%d(%s)%s", MKDISP(src1,off11),IREG[src2]
					,ainc);
didf0:
		dbstrcpy(operand[1], FREG[dest]);
		return 2;
	case rF_A:	/* freg,ireg-src1(ireg-src2)optainc */
		sprintf(operand[1], "%s(%s)%s", IREG[src1],IREG[src2],ainc);
		goto didf1;
	case iF_A:	/* freg,const-[src1:offset](ireg-src2)optinc */
		sprintf(operand[1], "%d(%s)%s", MKDISP(src1,off11),IREG[src2]
					,ainc);
didf1:
		dbstrcpy(operand[0], FREG[dest]);
		return 2;
	case R_F:	/* ireg-src1,freg-dest */
		dbstrcpy(operand[0], IREG[src1]);
		dbstrcpy(operand[1], FREG[dest]);
		return 2;
	case R_A:	/* ireg-src1,const-[dest:offset](ireg-src2) */
		dbstrcpy(operand[0], IREG[src1]);
		sprintf(operand[1], "%d(%s)", MKDISP(dest,off11), IREG[src2]);
		return 2;
	case iA_R:	/* const-[src1:offset],ireg-dest */
		sprintf(operand[0], "%d(%s)", MKDISP(src1,off11), IREG[src2]);
		goto did0;
	case rA_R:	/* ireg-src1(ireg-src2),ireg-dest */
		sprintf(operand[0], "%s(%s)", IREG[src1],IREG[src2]);
did0:
		dbstrcpy(operand[1], IREG[dest]);
		return 2;

	default:
	case UNKNOWN:
		sprintf(mnemonic,"***** Error - bad opcode\n");
		return 0;
	} /* end switch */
}

/*
 *	get_ins (high, low)
 *	Get the next byte and separate the op code into the high and
 *	low nibbles.
 */

static
get_ins(oct1,oct2, src2,dest,src1, off11, insn)
unsigned *oct1,*oct2;
unsigned *src2,*dest,*src1;
unsigned *off11;
unsigned insn;
{
	extern	unsigned short curbyte;		/* from _extn.c */
	extern	long	dot;
	unsigned b1,b2,b3,b4;
#if 0
	getbyte();	b1 = curbyte;
	getbyte();	b2 = curbyte;
	getbyte();	b3 = curbyte;
	getbyte();	b4 = curbyte;
#endif
	b4 = (insn >> 24) & 0xFF;
	b3 = (insn >> 16) & 0xFF;
	b2 = (insn >> 8) & 0xFF;
	b1 = insn & 0xFF;
/*
	printf("%02x%02x%02x%02x", b4,b3,b2,b1);
*/

	*oct1 = b4 >> 5;
	*oct2 = (b4 >> 2) & 07;

	*src2 = ((b4 & 03)<<3) | (b3 >> 5);
	*dest = b3 & 037;
	*src1 = b2 >> 3;

	*off11 = ((b2 & 07) << 8) | b1;

/*
	printf("\t%d-%d", *oct1,*oct2);
	if (*src1||*src2||*dest)
		printf(" %d,%d,%d", *src1,*src2,*dest);
	if (*off11)
		printf(" %d", *off11);
*/
}

static
mkd26(a,b,c,x)
unsigned a,b,c,x;
{
	long rv;

	rv = x | (c<<11) | (b<<16) | (a<<21);
	if (rv & 0x2000000)
		rv |= 0xfc000000;
	return rv;
}
