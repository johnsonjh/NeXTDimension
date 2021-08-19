/*
 * Internal data structures used by the pcode assembler.
 */
#include <nextdev/km_pcode.h>
struct vect
{
	int32 data;
	int32 addr;
};

/* Structure of a vector leaf node, crafted to match a generic leaf node */
struct vn
{
	int type;
	int count;
	struct vect *vp;
};

/* Structure of a generic leaf node */
struct ln
{
	int type;
	unsigned value;
	char *string;
};

/* structure of the generic tree node. */
struct tn
{
	int opcode;
	struct ln *l1;
	struct ln *l2;
	struct ln *l3;
	int	pc;
	int	line_no;
	char	*file_name;
	struct tn *next;
	struct tn *prev;
};

/* Structure of the internal symbol table used for label lookup. */
struct sym
{
	char *name;
	int flavor;
	int value;
	struct sym *next;
};
#define SYM_PC		0	/* Only supported flavor right now */

/* Defs for output processing flags */
#define OUT_LIST	0x0001

