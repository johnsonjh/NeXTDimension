%START COMMENT VECTLIST
%{
#include "pas.h"
#include "gram.tab.h"
extern int	CurrentNumber;
extern struct vect	*CurrentVect;
extern char	*CurrentFile;
%}
%%
"//".*$			;	/* Comments */
[ \t]			;	/* Whitespace consumer */
\n			return( SEMIOP );
";"			return( SEMIOP );
^"#"			return( CPP_OP );
"#"			return( (int)'#' );
"@"			return( (int) '@' );
","			return( (int) ',' );
":"			return( (int) ':' );
load			return( LOAD );
storev			return( STORE_VECT );
store			return( STORE );
end_storev		return( ENDSTOREV );
add			return( ADD );
sub			return( SUB );
and			return( AND );
or			return( OR );
xor			return( XOR );
asr			return( ASR );
asl			return( ASL );
move			return( MOVE );
test			return( TEST );
br			return( BRI );
bpos			return( BPOSI );
bneg			return( BNEGI );
bzero			return( BZEROI );
bnpos			return( BNPOSI );
bnneg			return( BNNEGI );
bnzero			return( BNZEROI );
end			return( ENDI );
".word"			return( WORD );
r[0-7]			{ CurrentNumber = yytext[1] - '0'; return( REGNO ); }
0x[0-9a-fA-F]+		{ sscanf( &yytext[2], "%x", &CurrentNumber );
			  return( NUMBER ); }
0[0-7]+			{ sscanf( yytext, "%o", &CurrentNumber );
			  return( NUMBER ); }
[1-9][0-9]*		{ CurrentNumber = atoi( yytext ); return( NUMBER ); }
0			{ CurrentNumber = 0; return( NUMBER ); }
[a-zA-Z_$][a-zA-Z0-9_$]*	return( STRING );
\"[^"\n]*\"		return( QSTRING );
.			return( (int) yytext[0] );


