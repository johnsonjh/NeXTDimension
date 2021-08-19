/*
 *	This is the header file for the iapx disassembler.
 *	The information contained in the first part of the file
 *	is common to each version, while the last part is dependent
 *	on the particular machine architecture being used.
 */

#define		NCPS	8	/* number of chars per symbol	*/
#define		NHEX	40	/* max # chars in object per line	*/
#define		NLINE	36	/* max # chars in mnemonic per line	*/
#define		FAIL	0
#if defined(TRUE)
#undef		TRUE
#endif
#if defined(FALSE)
#undef		FALSE
#endif
#define		TRUE	1
#define		FALSE	0
#define		LEAD	1
#define		NOLEAD	0
#define		NOLEADSHORT 2
#define		TERM 0		/* used in _tbls.c to indicate		*/
				/* that the 'indirect' field of the	*/
				/* 'instable' terminates - no pointer.	*/
				/* Is also checked in 'dis_text()' in	*/
				/* _bits.c.				*/

#define	LNNOBLKSZ	1024	/* size of blocks of line numbers	  */
#define	SYMBLKSZ	1024	/* size if blocks of symbol table entries */
#define	STRNGEQ		0	/* used in string compare operation	  */

/*
 *	This is the structure that will be used for storing all the
 *	op code information.  The structure values themselves are
 *	in '_tbls.c'.
 */

struct	instable {
	char		name[NCPS];
	struct instable *indirect;	/* for decode op codes */
	unsigned	adr_mode;
	unsigned	suffix;
};

/*	NOTE:	the following information in this file must be changed
 *		between the different versions of the disassembler.
 *
 *	This structure is used to determine the displacements and registers
 *	used in the addressing modes.  The values are in 'tables.c'.
 */
struct addr {
	int	disp;
	char	regs[9];
};

/*
 *	These are the instruction formats as they appear in
 *	'tbls.c'.  Here they are given numerical values
 *	for use in the actual disassembly of an object file.
 */

#define UNKNOWN		0
#define rA_R		1
#define iA_R		2
#define R_F		3
#define R_A		4
#define rA_F		5
#define iA_F		6
#define rF_A		7
#define iF_A		8
#define C_R		9
#define R_C		10
#define rS12D		11
#define S1		12
#define rS12B		13
#define iS12B		14
#define B26		15
#define iS12D		16
#define uS12D		17
#define fS12D		18
#define fS2D		19
#define fS1D		20
#define fS1Dr		21

/*
 *	kinds of size extersions
 */

#define	WL1		1
#define LDQ_A		2
#define M3		3
#define FLTS		4

/* float bits */

#define FLT_D		0x200
#define FLT_P		0x400
#define FLT_SD		0x100
#define FLT_RD		0x080
