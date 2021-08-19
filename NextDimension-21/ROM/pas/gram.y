%{
#include <stdio.h>
#include <nextdev/km_pcode.h>
#include "pas.h"

struct vn	CurrentVect;
int		CurrentNumber;
char		*CurrentFile;

extern void *malloc();
extern struct tn buildop();
extern int yylineno;

%}
%union {
	struct ln ln;
	struct vn vn;
	struct tn tn;
 }
 
 

%start	assemble
%type	<tn> ops
%token	<ln> LOAD STORE STORE_VECT ADD SUB AND OR XOR ASL ASR MOVE TEST
%token	<ln> BRI BPOSI BNEGI BZEROI BNPOSI BNNEGI BNZEROI ENDI WORD
%type	<ln> CONST ADDR REG TARGET CPP_SET_LINE QSTR
%token	<ln> STRING NUMBER  REGNO SEMIOP ENDSTOREV QSTRING CPP_OP
%type	<vn> VECT VECTLIST

%type	<ln> '#' '@' ','

%%	/* Rules section */
assemble	:	/* Empty */
		|	assemble ops
			{
			  extern char *CurrentFile;
			  $2.line_no = yylineno;
			  $2.file_name = CurrentFile;
			  add_to_forest( &$2 );
			}
		|	assemble SEMIOP
		|	assemble CPP_SET_LINE
		|	error SEMIOP
			{ yyerrok; }
		;
		
ops		:	LOAD CONST  ',' REG SEMIOP
			{ $$ =  buildop( LOAD_CR, &$2, &$4, NULL ); }
			
		|	LOAD ADDR ',' REG SEMIOP
			{ $$ = buildop( LOAD_AR, &$2, &$4, NULL ); }
			
		|	LOAD '@' REG ',' REG SEMIOP
			{ $$ = buildop( LOAD_IR, &$3, &$5, NULL ); }
		
		|	STORE REG ',' ADDR SEMIOP
			{ $$ = buildop( STORE_RA, &$2, &$4, NULL ); }
			
		|	STORE REG ',' '@' REG SEMIOP
			{ $$ = buildop( STORE_RI, &$2, &$5, NULL ); }
			
		|	STORE_VECT VECTLIST
			{ $$ = buildop( STOREV, &$2, NULL, NULL ); }
			
		|	STORE_VECT SEMIOP VECTLIST
			{ $$ = buildop( STOREV, &$3, NULL, NULL ); }
			
		|	ADD CONST ',' REG ',' REG SEMIOP
			{ $$ = buildop( ADD_CRR, &$2, &$4, &$6 ); }
			
		|	ADD REG ',' REG ',' REG SEMIOP
			{ $$ = buildop( ADD_RRR, &$2, &$4, &$6 ); }
			
		|	SUB REG ',' CONST ',' REG SEMIOP
			{ $$ = buildop( SUB_RCR, &$2, &$4, &$6 ); }
			
		|	SUB CONST ',' REG ',' REG SEMIOP
			{ $$ = buildop( SUB_CRR, &$2, &$4, &$6 ); }
			
		|	SUB REG ',' REG ',' REG SEMIOP
			{ $$ = buildop( SUB_RRR, &$2, &$4, &$6 ); }
			
		|	AND CONST ',' REG ',' REG SEMIOP
			{ $$ = buildop( AND_CRR, &$2, &$4, &$6 ); }
			
		|	AND REG ',' REG ',' REG SEMIOP
			{ $$ = buildop( AND_RRR, &$2, &$4, &$6 ); }
			
		|	OR CONST ',' REG ',' REG SEMIOP
			{ $$ = buildop( OR_CRR, &$2, &$4, &$6 ); }
			
		|	OR REG ',' REG ',' REG SEMIOP
			{ $$ = buildop( OR_RRR, &$2, &$4, &$6 ); }
			
		|	XOR CONST ',' REG ',' REG SEMIOP
			{ $$ = buildop( XOR_CRR, &$2, &$4, &$6 ); }
			
		|	XOR REG ',' REG ',' REG SEMIOP
			{ $$ = buildop( XOR_RRR, &$2, &$4, &$6 ); }

		|	ASL REG ',' CONST ',' REG SEMIOP
			{ $$ = buildop( ASL_RCR, &$2, &$4, &$6 ); }
		
		|	ASR REG ',' CONST ',' REG SEMIOP
			{ $$ = buildop( ASR_RCR, &$2, &$4, &$6 ); }
		
		|	MOVE REG ',' REG SEMIOP
			{ $$ = buildop( MOVE_RR, &$2, &$4, NULL ); }
			
		|	TEST REG SEMIOP
			{ $$ = buildop( TEST_R, &$2, NULL, NULL ); }

		|	BRI TARGET SEMIOP
			{ $$ = buildop( BR, &$2, NULL, NULL ); }
			
		|	BPOSI TARGET SEMIOP
			{ $$ = buildop( BPOS, &$2, NULL, NULL ); }

		|	BNEGI TARGET SEMIOP
			{ $$ = buildop( BNEG, &$2, NULL, NULL ); }

		|	BZEROI TARGET SEMIOP
			{ $$ = buildop( BZERO, &$2, NULL, NULL ); }

		|	BNPOSI TARGET SEMIOP
			{ $$ = buildop( BNPOS, &$2, NULL, NULL ); }

		|	BNNEGI TARGET SEMIOP
			{ $$ = buildop( BNNEG, &$2, NULL, NULL ); }

		|	BNZEROI TARGET SEMIOP
			{ $$ = buildop( BNZERO, &$2, NULL, NULL ); }
		
		|	ENDI SEMIOP
			{ $$ = buildop( END, NULL, NULL, NULL ); }
		
		|	WORD	TARGET SEMIOP
			{ $$ = buildop( WORD, &$2, NULL, NULL ); }
			
		|	WORD	CONST SEMIOP
			{ $$ = buildop( WORD, &$2, NULL, NULL ); }
			
		|	TARGET ':'
			{ $$ = buildop( LABEL_T, &$1, NULL, NULL ); }
		;
		
TARGET		:	STRING
			{ extern char yytext[];
			  extern char *savestr();
			  $$.type = TARGET_T;
			  $$.string = savestr( yytext );
			}
		;
VECTLIST	:	VECT ENDSTOREV SEMIOP
			{ $$ = CurrentVect;
			  CurrentVect.count = 0;
			  CurrentVect.vp = (struct vect *) NULL;
			}
		;
VECT		:	VECT	SEMIOP
		|	VECT CONST ',' ADDR	SEMIOP
			{ CurrentVect.count++;
			  if ( CurrentVect.vp == (struct vect *)NULL )
			  {  CurrentVect.vp = (struct vect *)
			       malloc(CurrentVect.count * sizeof(struct vect));
			  }
			  else
			  {  CurrentVect.vp = (struct vect *)
			       realloc( CurrentVect.vp, 
			       		CurrentVect.count*sizeof(struct vect));
			  }
			  if ( CurrentVect.vp == (struct vect *)NULL )
			  {
			  	fprintf(stderr, "Line %d: Out of memory. ",
					yylineno );
				fprintf(stderr,
					"Try breaking up the storev list\n" );
				exit( 1 );
			  }
			  CurrentVect.vp[CurrentVect.count-1].data = $2.value;
			  CurrentVect.vp[CurrentVect.count-1].addr = $4.value;
			}
		|	CONST ',' ADDR SEMIOP
			{ CurrentVect.count = 1;
			  if ( CurrentVect.vp == (struct vect *)NULL )
			  {  CurrentVect.vp = (struct vect *)
			       malloc(sizeof(struct vect));
			  }
			  else
			  {  CurrentVect.vp = (struct vect *)
			       realloc(CurrentVect.vp, sizeof(struct vect));
			  }
			  if ( CurrentVect.vp == (struct vect *)NULL )
			  {
			  	fprintf(stderr, "Line %d: Out of memory. ",
					yylineno );
				fprintf(stderr,
					"Try breaking up the storev list\n" );
				exit( 1 );
			  }
			  CurrentVect.vp->data = $1.value;
			  CurrentVect.vp->addr = $3.value;
			}
		;

CPP_SET_LINE	:	CPP_OP	NUMBER	QSTR
			{
				CurrentFile = $3.string;
				yylineno = CurrentNumber - 1;
			}
		;
ADDR		:	'@' NUMBER
			{ if (  (CurrentNumber & 0xF0000000) != 0
			     && (CurrentNumber & 0xFF000000) != 0xF0000000)
			  	yyerror( "Illegal address" );
			  $$.value = CurrentNumber;
			  $$.type = ADDR_T;
			  $$.string = (char *) NULL;
			}
		;
CONST		:	'#' NUMBER
			{ $$.type = CONST_T;
			  $$.value = CurrentNumber;
			  $$.string = (char *) NULL;
			}
		|	NUMBER
			{ $$.type = CONST_T;
			  $$.value = CurrentNumber;
			  $$.string = (char *) NULL;
			}
		;
		
REG		:	REGNO
			{ $$.type = REG_T;
			  $$.value = CurrentNumber;
			  $$.string = (char *) NULL;
			}
		;
QSTR		:	QSTRING
			{	extern char yytext[];
				extern char *savestr();
				$$.string = savestr( yytext );
			}
		;

%%

