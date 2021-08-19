/*
 * Listing function for the assembler.
 */
#include <stdio.h>
#include <sys/file.h>
#include <nextdev/km_pcode.h>
#include "pas.h"
#include "gram.tab.h" 


list( tn, ops)
    struct tn *tn;
    int32 ops[];
{
	if ( IMMEDIATE_FOLLOWS_OPCODE(ops[0])  && tn->opcode != WORD )
		printf( "%6x: %08x %08x\t\t", tn->pc, ops[0], ops[1] );
	else
		printf( "%6x: %08x         \t\t", tn->pc, ops[0] );

	switch( tn->opcode )
	{
		case LOAD_CR:
			printf( "load\t#0x%08x, r%d\n", ops[1], REG2(ops[0]) );
			break;
		case LOAD_AR:
			printf( "load\t@0x%08x, r%d\n", ops[1], REG2(ops[0]) );
			break;
		case LOAD_IR:
			printf("load\t@r%d, r%d\n",REG1(ops[0]),REG2(ops[0]));
			break;
		case STORE_RA:
			printf( "store\tr%d, @0x%08x\n", REG1(ops[0]), ops[1]);
			break;
		case STORE_RI:
			printf("store\tr%d, @r%d\n",REG1(ops[0]),REG2(ops[0]));
			break;
		case STOREV:
		{
			int32 count;
			struct vect *vp;
			int pc;
			int i;
			
			count = ((struct vn *)tn->l1)->count;
			vp = ((struct vn *)tn->l1)->vp;
			pc = tn->pc + 1;
			
			printf( "storev\n" );
			i = 0;
			while( count-- )
			{
				printf( "%6x: %08x %08x\t\t",
						pc, vp[i].data, vp[i].addr );
				printf( "\t#0x%08x, @0x%08x;\n",
					vp[i].data, vp[i].addr );
				++i;
				pc += 2;
			}
			printf( "\t\t\t\t\tend_storev\n" );
			break;
		}
			
		case ADD_CRR:
			printf( "add\t#0x%08x, r%d, r%d\n",
				ops[1],REG1(ops[0]),REG2(ops[0]) );
			break;
		case XOR_CRR:
			printf( "xor\t#0x%08x, r%d, r%d\n",
				ops[1],REG1(ops[0]),REG2(ops[0]) );
			break;

		case OR_CRR:
			printf( "or\t#0x%08x, r%d, r%d\n",
				ops[1],REG1(ops[0]),REG2(ops[0]) );
			break;

		case AND_CRR:
			printf( "and\t#0x%08x, r%d, r%d\n",
				ops[1],REG1(ops[0]),REG2(ops[0]) );
			break;

		case SUB_CRR:
			printf( "sub\t#0x%08x, r%d, r%d\n",
				ops[1],REG1(ops[0]),REG2(ops[0]) );
			break;

		case ADD_RRR:
			printf( "add\tr%d, r%d, r%d\n",
				REG1(ops[0]),REG2(ops[0]),REG3(ops[0]) );
			break;

		case SUB_RRR:
			printf( "sub\tr%d, r%d, r%d\n",
				REG1(ops[0]),REG2(ops[0]),REG3(ops[0]) );
			break;

		case AND_RRR:
			printf( "and\tr%d, r%d, r%d\n",
				REG1(ops[0]),REG2(ops[0]),REG3(ops[0]) );
			break;

		case OR_RRR:
			printf( "or\tr%d, r%d, r%d\n",
				REG1(ops[0]),REG2(ops[0]),REG3(ops[0]) );
			break;

		case XOR_RRR:
			printf( "xor\tr%d, r%d, r%d\n",
				REG1(ops[0]),REG2(ops[0]),REG3(ops[0]) );
			break;
			
		case SUB_RCR:
			printf( "sub\tr%d, #0x%08x, r%d\n",
				REG1(ops[0]), ops[1], REG2(ops[0]));
			break;

		case ASL_RCR:
			printf( "asl\tr%d, #%d, r%d\n",
				REG1(ops[0]), FIELD2(ops[0]), REG3(ops[0]) );
			break;

		case ASR_RCR:
			printf( "asr\tr%d, #%d, r%d\n",
				REG1(ops[0]), FIELD2(ops[0]), REG3(ops[0]) );
			break;
			
		case MOVE_RR:
			printf( "move\tr%d, r%d\n", REG1(ops[0]),REG2(ops[0]));
			break;
			
		case TEST_R:
			printf( "test\tr%d\n", REG1(ops[0]) );
			break;
			
		case BR:
			printf( "br\t%6x\n", BRANCH_DEST(ops[0]) );
			break;
			
		case BPOS:
			printf( "bpos\t%6x\n", BRANCH_DEST(ops[0]) );
			break;

		case BNEG:
			printf( "bneg\t%6x\n", BRANCH_DEST(ops[0]) );
			break;

		case BZERO:
			printf( "bzero\t%6x\n", BRANCH_DEST(ops[0]) );
			break;

		case BNPOS:
			printf( "bnpos\t%6x\n", BRANCH_DEST(ops[0]) );
			break;

		case BNNEG:
			printf( "bnneg\t%6x\n", BRANCH_DEST(ops[0]) );
			break;

		case BNZERO:
			printf( "bnzero\t%6x\n", BRANCH_DEST(ops[0]) );
			break;

		case END:
			printf( "end\n" );
			break;
			
		case WORD:
			printf( ".word\t0x%08x\n", ops[0] );
			break;
			
		default:
			fprintf( stderr, "Unexpected opcode %d!\n",
				tn->opcode );
			fatal( "Ungrokkable opcode!" );
	}
}


