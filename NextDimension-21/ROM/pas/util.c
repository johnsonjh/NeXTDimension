/*
 * Utility functions for the assembler.
 */
#include <stdio.h>
#include "pas.h"
#include "gram.tab.h" 

static void flush_syms();
/*
 * savestr:
 *
 *	Allocate storage and copy the string to the new store.
 *	return a pointer to the copy.
 *
 * Error:
 *	Out of memory error forces an exit.
 */
 char *
savestr( s )
    char *s;
{
	char *copy;
	extern void *malloc();
	
	if ( (copy = (char *) malloc( strlen(s) + 1 )) == (char *) NULL )
		fatal( "Out of Memory" );
	strcpy( copy, s );
	return( copy );
}

/*
 * Allocate storage for the tree node.  Copy the tree into the storage,
 * and add the tree to the Forest.
 *
 * (I never wanted to be a programmer.  I wanted to be... a lumberjack!)
 */
add_to_forest( node )
    struct tn *node;
{
	extern struct tn *Forest;
	struct tn *copy;
	extern void *malloc();
	
	if ( (copy = malloc( sizeof(struct tn) )) == (struct tn *) NULL )
		fatal( "Out of Memory" );
	*copy = *node;	/* copy the structure */
	
	if ( Forest == (struct tn *) NULL )
	{	/* Start the doubly linked list */
		Forest = copy;
		copy->next = copy;
		copy->prev = copy;
	}
	else
	{	/* tail insert the structure on the list */
		copy->prev = Forest->prev;
		copy->next = Forest;
		Forest->prev->next = copy;
		Forest->prev = copy;
	}
}

/*
 * deforest():
 *
 *	Free all of the storage in the forest.
 *	Flush the symbol table.
 */
deforest()
{
	extern struct tn* Forest;
	register struct tn *fp = Forest;
	register struct tn *next;
	
	if ( Forest == (struct tn *) NULL )
		return;
	do
	{
		next = fp->next;
		if ( fp->l1 != (struct ln *) NULL )
		{
			if ( fp->l1->string != (char *) NULL )
				free( (void *) fp->l1->string );
			free( (void *) fp->l1 );
		}
		if ( fp->l2 != (struct ln *) NULL )
		{
			if ( fp->l2->string != (char *) NULL )
				free( (void *) fp->l2->string );
			free( (void *) fp->l2 );
		}
		if ( fp->l3 != (struct ln *) NULL )
		{
			if ( fp->l3->string != (char *) NULL )
				free( (void *) fp->l3->string );
			free( (void *) fp->l3 );
		}
		free( (void *)fp );
		fp = next;
	} while ( fp != Forest );
	Forest = (struct tn *) NULL;
	flush_syms();	/* Zap symbol table while we are at it. */
}

/*
 * buildop:
 *	Given an opcode and a set of leaf nodes,
 *	construct and return a tree node structure.
 */
 struct tn
buildop( op, l1, l2, l3 )
    int op;
    struct ln *l1;
    struct ln *l2;
    struct ln *l3;
{
	struct tn node;
	extern struct ln *saveleaf();
	
	node.opcode = op;
	if ( l1 != (struct ln *) NULL )
		node.l1 = saveleaf(l1);
	else
		node.l1 = (struct ln *) NULL;
		
	if ( l2 != (struct ln *) NULL )
		node.l2 = saveleaf(l2);
	else
		node.l2 = (struct ln *) NULL;
		
	if ( l3 != (struct ln *) NULL )
		node.l3 = saveleaf(l3);
	else
		node.l3 = (struct ln *) NULL;

	return( node );
}

 struct ln *
saveleaf( lp )
    struct ln *lp;
{
	struct ln *copy;
	extern void *malloc();
	
	if ( (copy = (struct ln *)malloc( sizeof (struct ln) )) == NULL )
		fatal( "Out of Memory" );
	*copy = *lp;	/* copy struct */
	
	return( copy );
}

/*
 * A miniature symbol table manager.
 *
 * lookup(string) returns a pointer to the symbol entry.
 * install(name, value, flavor) enters a value in the symbol table.
 * flush_syms() clears out the symbol table.
 */
#define HASHSIZE	101	/* use a prime number... */
static struct sym *hashtab[HASHSIZE];

 struct sym *
lookup( string )
    char *string;
{
	int sum;
	char *cp;
	char *cp2;
	struct sym *sp;
	
	/* suboptimal but fast hash and lookup */
	for ( sum = 0, cp = string; *cp; sum += *cp++ )
		;
	sum %= HASHSIZE;
	for( sp = hashtab[sum]; sp != (struct sym *) NULL; sp = sp->next )
	{
		cp = string;
		cp2 = sp->name;
		while( *cp++ == *cp2 )
		{
			if ( *cp2++ == '\0' ) /* Match and end of string */
				return( sp );
		}
	}
	return( (struct sym *) NULL );
}

/* 
 * return 1 if the symbol is already defined.
 * return 0 if this is a new symbol.
 */
install( string, value, flavor )
    char *string;
    int value;
    int flavor;
{
	int sum;
	struct sym *sp;
	char *cp;
	extern void *malloc();
	
	/* If already present, just update and leave */
	if ( (sp = lookup( string )) != (struct sym *) NULL )
	{
		sp->value = value;
		sp->flavor = flavor;
		return 1;
	}
	/* suboptimal but fast hash */
	for ( sum = 0, cp = string; *cp; sum += *cp++ )
		;
	sum %= HASHSIZE;
	
	if ( (sp = (struct sym *) malloc( sizeof(struct sym) )) == NULL )
		fatal( "Out of Memory" );
	
	sp->name = savestr( string );
	sp->value = value;
	sp->flavor = flavor;
	sp->next = hashtab[sum];
	hashtab[sum] = sp;
	return( 0 );
}

 static void
flush_syms()
{
	int i;
	struct sym *sp;
	struct sym *next;
	
	for ( i = 0; i < HASHSIZE; ++i )
	{	
		sp = hashtab[i];
		while( sp != (struct sym *) NULL )
		{
			next = sp->next;
			free( (void *) sp );
			sp = next;
		}
		hashtab[i] = (struct sym *) NULL;
	}
}
