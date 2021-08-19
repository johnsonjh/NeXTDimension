/*
 * Definition file for video ROM pcode opcodes.
 */
#ifndef _PTOKENS_H_
#define _PTOKENS_H_	1

/*
 *	Token defs for use in the lexical analyzer, 
 *	one per keyword or primitive construct.
 */
#define LOAD		1
#define STORE		2
#define STOREV		3
#define ENDSTOREV	4
#define	ADD		5
#define	SUB		6
#define	AND		7
#define	OR		8
#define	XOR		9
#define ASR		10
#define ASL		20
#define	MOVE		11
#define	TEST		12
#define	BR		13
#define BPOS		14
#define BNEG		15
#define	BZERO		16
#define BNPOS		17
#define BNNEG		18
#define BNZERO		19
#define END		0

/*	Operand tokens.	*/
#define REG		21
#define	STRING		22
#define IMMED		23	/* The # character */
#define NUMBER		24	/* Base 10 number */
#define HEXNUM		25
#define OCTNUM		26
#define COLON		27

#endif /* _PTOKENS_H_ */
