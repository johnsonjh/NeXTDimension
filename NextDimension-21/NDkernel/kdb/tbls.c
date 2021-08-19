#include	"dis.h"

#define		INVALID	{"",TERM,UNKNOWN,0}

char	*IREG[32] = {
	"r0", "r1", "sp", "fp",
	"r4", "r5", "r6", "r7",
	"r8", "r9", "r10", "r11",
	"r12", "r13", "r14", "r15",
	"r16", "r17", "r18", "r19",
	"r20", "r21", "r22", "r23",
	"r24", "r25", "r26", "r27",
	"r28", "r29", "r30", "r31"
};

char	*FREG[32] = {
	"f0", "f1", "f2", "f3",
	"f4", "f5", "f6", "f7",
	"f8", "f9", "f10", "f11",
	"f12", "f13", "f14", "f15",
	"f16", "f17", "f18", "f19",
	"f20", "f21", "f22", "f23",
	"f24", "f25", "f26", "f27",
	"f28", "f29", "f30", "f31"
};

/*
 * Special Registers
 */

char *CONTROLREG[] = {
	"fir", "psr", "dirbase", "db", "fsr",
	"?", "?", "?",
	"?", "?", "?", "?", "?", "?", "?", "?",
	"?", "?", "?", "?", "?", "?", "?", "?",
	"?", "?", "?", "?", "?", "?", "?", "?",
};

struct instable flttbl[128] = {
/* 0x00 */	{"r2p1",TERM,fS12D,FLTS},	{"r2pt",TERM,fS12D,FLTS},
/* 0x02 */	{"r2ap1",TERM,fS12D,FLTS},	{"r2apt",TERM,fS12D,FLTS},
/* 0x04 */	{"i2p1",TERM,fS12D,FLTS},	{"i2pt",TERM,fS12D,FLTS},
/* 0x06 */	{"i2ap1",TERM,fS12D,FLTS},	{"i2apt",TERM,fS12D,FLTS},
/* 0x08 */	{"rat1p2",TERM,fS12D,FLTS},	{"m12apm",TERM,fS12D,FLTS},
/* 0x0a */	{"ra1p2",TERM,fS12D,FLTS},	{"m12ttpa",TERM,fS12D,FLTS},
/* 0x0c */	{"iat1p2",TERM,fS12D,FLTS},	{"m12tmp",TERM,fS12D,FLTS},
/* 0x0e */	{"ia1p2",TERM,fS12D,FLTS},	{"m12tpa",TERM,fS12D,FLTS},

/* 0x10 */	{"r2s1",TERM,fS12D,FLTS},	{"r2st",TERM,fS12D,FLTS},
/* 0x12 */	{"r2as1",TERM,fS12D,FLTS},	{"r2ast",TERM,fS12D,FLTS},
/* 0x14 */	{"i2s1",TERM,fS12D,FLTS},	{"i2st",TERM,fS12D,FLTS},
/* 0x16 */	{"i2as1",TERM,fS12D,FLTS},	{"i2ast",TERM,fS12D,FLTS},
/* 0x18 */	{"rat1s2",TERM,fS12D,FLTS},	{"m12asm",TERM,fS12D,FLTS},
/* 0x1a */	{"ra1s2",TERM,fS12D,FLTS},	{"m12ttsa",TERM,fS12D,FLTS},
/* 0x1c */	{"iat1s2",TERM,fS12D,FLTS},	{"m12tms",TERM,fS12D,FLTS},
/* 0x1e */	{"ia1s2",TERM,fS12D,FLTS},	{"m12tsa",TERM,fS12D,FLTS},

/* 0x20 */	{"fmul",TERM,fS12D,FLTS},	{"fmlow",TERM,fS12D,FLTS},
/* 0x22 */	{"frcp",TERM,fS2D,FLTS},	{"frsqr",TERM,fS2D,FLTS},
/* 0x24 */	INVALID,			INVALID,
/* 0x26 */	INVALID,			INVALID,
/* 0x28 */	INVALID,			INVALID,
/* 0x2a */	INVALID,			INVALID,
/* 0x2c */	INVALID,			INVALID,
/* 0x2e */	INVALID,			INVALID,

/* 0x30 */	{"fadd",TERM,fS12D,FLTS},	{"fsub",TERM,fS12D,FLTS},
/* 0x32 */	{"fix",TERM,fS1D,FLTS},		{"famov",TERM,fS1D,FLTS},
/* 0x34 */	{"fgt",TERM,fS12D,FLTS},	{"feq",TERM,fS12D,FLTS},
/* 0x36 */	INVALID,			INVALID,
/* 0x38 */	INVALID,			INVALID,
/* 0x3a */	{"ftrunc",TERM,fS1D,FLTS},	INVALID,
/* 0x3c */	INVALID,			INVALID,
/* 0x3e */	INVALID,			INVALID,

/* 0x40 */	{"fxfr",TERM,fS1Dr,FLTS},	INVALID,
/* 0x42 */	INVALID,			INVALID,
/* 0x44 */	INVALID,			INVALID,
/* 0x46 */	INVALID,			INVALID,
/* 0x48 */	INVALID,			{"fiadd",TERM,fS12D,FLTS},
/* 0x4a */	INVALID,			INVALID,
/* 0x4c */	INVALID,			{"fisub",TERM,fS12D,FLTS},
/* 0x4e */	INVALID,			INVALID,

/* 0x50 */	{"faddp",TERM,fS12D,FLTS},	{"faddz",TERM,fS12D,FLTS},
/* 0x52 */	INVALID,			INVALID,
/* 0x54 */	INVALID,			INVALID,
/* 0x56 */	INVALID,			{"fzchkl",TERM,fS12D,FLTS},
/* 0x58 */	INVALID,			INVALID,
/* 0x5a */	{"form",TERM,fS1D,FLTS},	INVALID,
/* 0x5c */	INVALID,			INVALID,
/* 0x5e */	INVALID,			{"fzchks",TERM,fS12D,FLTS},

	{0}
};

/*
 *	Main decode table for the op codes.  The first two octal digits
 *	will be used as an index into the table.  If there is a
 *	a need to further decode an instruction, the array to be
 *	referenced is indicated with the other two entries being
 *	empty.
 */

struct instable distable[64] = {

/* [0,0] */	{"ld.b",TERM,rA_R,0},		{"ld.b",TERM,iA_R,0},
/* [0,2] */	{"ixfr",TERM,R_F,0},		{"st.b",TERM,R_A,0},
/* [0,4] */	{"ld",TERM,rA_R,WL1},		{"ld",TERM,iA_R,WL1},
/* [0,6] */	INVALID,			{"st",TERM,R_A,WL1},

/* [1,0] */	{"fld",TERM,rA_F,LDQ_A},	{"fld",TERM,iA_F,LDQ_A},
/* [1,2] */	{"fst",TERM,rF_A,LDQ_A},	{"fst",TERM,iF_A,LDQ_A},
/* [1,4] */	{"ld.c",TERM,C_R,0},		{"flush",TERM,iA_F,M3},
/* [1,6] */	{"st.c",TERM,R_C,0},		{"pst.d",TERM,iF_A,M3},

/* [2,0] */	{"bri",TERM,S1,0},		{"trap",TERM,rS12D,0},
/* [2,2] */	{"",flttbl,0,0},		INVALID,
/* [2,4] */	{"btne",TERM,rS12B,0},		{"btne",TERM,iS12B,0},
/* [2,6] */	{"bte",TERM,rS12B,0},		{"bte",TERM,iS12B,0},

/* [3,0] */	{"p.fld",TERM,rA_F,LDQ_A},	{"p.fld",TERM,iA_F,LDQ_A},
/* [3,2] */	{"br",TERM,B26,0},		{"call",TERM,B26,0},
/* [3,4] */	{"bc",TERM,B26,0},		{"bc.t",TERM,B26,0},
/* [3,6] */	{"bnc",TERM,B26,0},		{"bnc.t",TERM,B26,0},

/* [4,0] */	{"addu",TERM,rS12D,0},		{"addu",TERM,iS12D,0},
/* [4,2] */	{"subu",TERM,rS12D,0},		{"subu",TERM,iS12D,0},
/* [4,4] */	{"adds",TERM,rS12D,0},		{"adds",TERM,iS12D,0},
/* [4,6] */	{"subs",TERM,rS12D,0},		{"subs",TERM,iS12D,0},

/* [5,0] */	{"shl",TERM,rS12D,0},		{"shl",TERM,iS12D,0},
/* [5,2] */	{"shr",TERM,rS12D,0},		{"shr",TERM,iS12D,0},
/* [5,4] */	{"shrd",TERM,rS12D,0},		{"bla",TERM,rS12B,0},
/* [5,6] */	{"shra",TERM,rS12D,0},		{"shra",TERM,iS12D,0},

/* [6,0] */	{"and",TERM,rS12D,0},		{"and",TERM,uS12D,0},
/* [6,2] */	INVALID,			{"andh",TERM,uS12D,0},
/* [6,4] */	{"andnot",TERM,rS12D,0},	{"andnot",TERM,uS12D,0},
/* [6,6] */	INVALID,			{"andnoth",TERM,uS12D,0},

/* [7,0] */	{"or",TERM,rS12D,0},		{"or",TERM,uS12D,0},
/* [7,2] */	INVALID,			{"orh",TERM,uS12D,0},
/* [7,4] */	{"xor",TERM,rS12D,0},		{"xor",TERM,uS12D,0},
/* [7,6] */	INVALID,			{"xorh",TERM,uS12D,0},

};
