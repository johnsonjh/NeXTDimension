/*
 * Pass 1 for the assembler.
 */
#include <stdio.h>
#include <nextdev/km_pcode.h>
#include "pas.h"
#include "gram.tab.h" 

/*
 * pass1():
 *
 *	Really the second pass, since the lexer and parser conducted the 
 *	first pass. Anyway, buzz through the forest and assign PCs to
 *	each real instruction.  Collect the labels into the 'symbol table'.
 */
pass1()
{
	extern struct tn *Forest;
	struct tn *fp = Forest;
	int pc	= 0;
	char buffer[256];
	
	if ( fp == (struct tn *) NULL )
		return;
		
	do
	{
		switch( fp->opcode )
		{
			case LABEL_T:
				fp->pc = pc;
				if ( install( fp->l1->string, pc, SYM_PC ) )
				{
				    sprintf( buffer, "label \"%s\" redefined",
				    		fp->l1->string );
				    misc_warning(buffer,
				  		fp->file_name,fp->line_no);
				}
				break;
				
			case WORD:
				fp->pc = pc++;
				break;
				
			case STOREV:
				fp->pc = pc++;
				pc += (((struct vn *)fp->l1)->count) * 2;
				break;
				
			default:
				fp->pc = pc;
				if ( (fp->opcode) & OPCODE_IMMED_BIT )
					pc += 2;
				else
					++pc;
				break;
		}
		fp = fp->next;
	}
	while( fp != Forest );
}
