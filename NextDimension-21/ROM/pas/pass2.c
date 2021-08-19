/*
 * Pass 2 for the assembler.
 */
#include <stdio.h>
#include <nextdev/km_pcode.h>
#include "pas.h"
#include "gram.tab.h" 

/*
 * For each leaf of type TARGET_T, fill in the leaf's value field.
 */
pass2()
{
	extern struct tn *Forest;
	struct tn *fp = Forest;
	struct sym *sp;
	extern struct sym *must_lookup();
	
	if ( fp == (struct tn *) NULL )
		return;
		
	do
	{
		if ( fp->l1 != (struct ln *) NULL && fp->l1->type == TARGET_T )
		{
			 sp = must_lookup( fp->l1->string, fp->line_no );
			 if ( sp != NULL )
				fp->l1->value = sp->value;
		}
		if ( fp->l2 != (struct ln *) NULL && fp->l2->type == TARGET_T )
		{
			 sp = must_lookup( fp->l2->string, fp->line_no );
			 if ( sp != NULL )
				fp->l2->value = sp->value;
		}
		if ( fp->l3 != (struct ln *) NULL && fp->l3->type == TARGET_T )
		{
			 sp = must_lookup( fp->l3->string, fp->line_no );
			 if ( sp != NULL )
				fp->l3->value = sp->value;
		}
		fp = fp->next;
	}
	while( fp != Forest );
}
 struct sym *
must_lookup( str, line )
    char *str;
    int	line;
{
	char buff[256];
	struct sym *sp;
	extern struct sym *lookup();
	extern char *CurrentFile;
	
	if ( (sp = lookup( str )) != (struct sym *) NULL )
		return( sp );
	
	(void) sprintf( buff, "Unknown symbol \"%s\"", str );
	misc_error( buff, CurrentFile, line );
	return( (struct sym *) NULL );
}
