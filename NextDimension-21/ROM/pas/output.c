/*
 * Output functions for the assembler.
 */
#include <stdio.h>
#include <sys/file.h>
#include <nextdev/km_pcode.h>
#include "pas.h"
#include "gram.tab.h" 

static int assemble();

static void writesetup();
static void writeout();
static void writeflush();

output( name, flags )
    char * name;
    int flags;
{
	extern struct tn *Forest;
	struct tn *fp = Forest;
	int32	ops[2];
	int count;
	int ofd;

	if ( flags & OUT_LIST )
		printf( "//  Assembly of %s\n", name );
	
	if ( name == (char *) NULL )
		name = "pas.out";
			
	if ( (ofd = open( name, O_WRONLY|O_CREAT|O_TRUNC, 0666 )) == -1 )
	{
		perror( name );
		fatal( "Cannot create output file." );
	}
	
	writesetup( ofd );
	
	do
	{
		if ( fp->opcode != LABEL_T )
		{
			count = assemble( fp, ops, ofd );/* Assemble 1 inst. */
			if ( flags & OUT_LIST )
				list( fp, ops );	/* List the output */
			
		}
		fp = fp->next;
	}
	while( fp != Forest );
	writeflush( ofd );
	close( ofd );
}

 static int
assemble( tn, ops, ofd )
    struct tn *tn;
    int32 ops[];
    int ofd;
{
	ops[0] = ops[1] = 0;
	SET_OPCODE(ops[0], tn->opcode);
	
	switch( tn->opcode )
	{
		case LOAD_CR:
			SET_REG2(ops[0], tn->l2->value);
			ops[1] = tn->l1->value;
			writeout( ops, 2, ofd );
			break;
		case LOAD_AR:
			SET_REG2(ops[0], tn->l2->value);
			ops[1] = tn->l1->value;
			writeout( ops, 2, ofd );
			break;
		case LOAD_IR:
			SET_REG2(ops[0], tn->l2->value);
			SET_REG1(ops[0], tn->l1->value);
			writeout( ops, 1, ofd );
			break;
		case STORE_RA:
			SET_REG1(ops[0], tn->l1->value);
			ops[1] = tn->l2->value;
			writeout( ops, 2, ofd );
			break;
		case STORE_RI:
			SET_REG2(ops[0], tn->l2->value);
			SET_REG1(ops[0], tn->l1->value);
			writeout( ops, 1, ofd );
			break;
		case STOREV:
		{
			int32 count;
			struct vect *vp;
			
			count = ((struct vn *)tn->l1)->count;
			vp = ((struct vn *)tn->l1)->vp;
			
			SET_BRANCH_DEST( ops[0], count );
			writeout( ops, 1, ofd );
			writeout( vp, count * 2, ofd );
			break;
		}
			
		case ADD_CRR:
		case XOR_CRR:
		case OR_CRR:
		case AND_CRR:
		case SUB_CRR:
			ops[1] = tn->l1->value;
			SET_REG1( ops[0], tn->l2->value );
			SET_REG2( ops[0], tn->l3->value );
			writeout( ops, 2, ofd );
			break;
			
		case ADD_RRR:
		case SUB_RRR:
		case AND_RRR:
		case OR_RRR:
		case XOR_RRR:
			SET_REG1( ops[0], tn->l1->value );
			SET_REG2( ops[0], tn->l2->value );
			SET_REG3( ops[0], tn->l3->value );
			writeout( ops, 1, ofd );
			break;
			
		case SUB_RCR:
			ops[1] = tn->l2->value;
			SET_REG1( ops[0], tn->l1->value );
			SET_REG2( ops[0], tn->l3->value );
			writeout( ops, 2, ofd );
			break;
			
		case ASL_RCR:
		case ASR_RCR:
			SET_FIELD2(ops[0], tn->l2->value );
			SET_REG1( ops[0], tn->l1->value );
			SET_REG3( ops[0], tn->l3->value );
			writeout( ops, 1, ofd );

			break;
			
		case MOVE_RR:
			SET_REG1( ops[0], tn->l1->value );
			SET_REG2( ops[0], tn->l2->value );
			writeout( ops, 1, ofd );
			break;
			
		case TEST_R:
			SET_REG1( ops[0], tn->l1->value );
			writeout( ops, 1, ofd );
			break;
			
		case BR:
		case BPOS:
		case BNEG:
		case BZERO:
		case BNPOS:
		case BNNEG:
		case BNZERO:
			SET_BRANCH_DEST( ops[0], tn->l1->value );
			writeout( ops, 1, ofd );
			break;
			
		case END:
			writeout( ops, 1, ofd );
			break;
			
		case WORD:
			ops[0] = tn->l1->value;
			writeout( ops, 1, ofd );
			break;
			
		default:
			fprintf( stderr, "Unexpected opcode %d!\n",
				tn->opcode );
			fatal( "Ungrokkable opcode!" );
	}
}

/*
 * The following constitutes a very simple-minded buffered I/O
 * system.  We don't use fwrite(), because of ill-defined byte order
 * behavior on different machines when writing blocks of words.
 */
#define WBUFSIZE	256
static int32 WriteBuf[WBUFSIZE];
static int WriteBufCnt;

 static void
writesetup( ofd )
    int ofd;
{
	WriteBufCnt = 0;
}

 static void
writeout( ops, len, ofd )
    int32 *ops;
    int len;
    int ofd;
{
	/* On entry, there is guaranteed to be room for at least one
	 * word in the buffer.
	 */
	while( len-- )
	{
		WriteBuf[WriteBufCnt++] = *ops++;
		if ( WriteBufCnt == WBUFSIZE )
			writeflush( ofd );
	}
}

 static void
writeflush( ofd )
    int ofd;
{
	int cnt = WriteBufCnt * sizeof WriteBuf[0];
	if ( write( ofd, WriteBuf, cnt ) != cnt )	/* Questionable... */
	{
		perror( "Write" );
		fatal( "Output write fails" );
	}
}



