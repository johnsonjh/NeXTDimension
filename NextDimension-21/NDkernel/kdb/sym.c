/* 
 * Mach Operating System
 * Copyright (c) 1987 Carnegie-Mellon University
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 */
/*
 ****************************************************************
 * HISTORY
 *  3-Oct-87  Michael Young (mwyoung) at Carnegie-Mellon University
 *	De-linted.
 *
 *  7-Nov-86  David Golub (dbg) at Carnegie-Mellon University
 *	Made findsym ignore symbols outside kernel if kernel debugger -
 *	avoids printing 'INCLUDE_VERSION' for stack offsets.
 *
 ****************************************************************
 */
/*
 * adb - symbol table routines
 */
#include "defs.h"
#ifdef i860
#define ALIGN(p)	(p += (long)p&3 ? 4-((long)p&3) : 0)
#else
#define ALIGN(p)
#endif

/*
 * findsym looks thru symtable to find the routine name which begins
 * closest to value and returns the difference between value
 * and the symbol found.  Leaves a pointer to the symbol found
 * in cursym.  The format of symtable is a list of
 * (long, null-terminated string) pairs; the list is sorted
 * on the longs.  symtable is populated by patching the kernel using
 * a program called unixsyms.
 */

#define SYMSIZE	50000
#ifndef FOR_DB
char symtable[SYMSIZE] = {0};     /* initialize to get it into .data section */
#else
extern char *symtable;
#endif

/* struct nlist tmpsym = 0; */
struct nlist tmpsym;	/* No, sorry, that won't work. */

findsym(value, tell)
unsigned long value;
int tell;   /* flag to print name and location */
{
	extern int cons_simul;
	extern struct nlist *cursym;
	char *p = symtable;
	char *oldp = p;

	if (cons_simul) {
		cursym = &tmpsym;
		return(value);
	}
	while (*(unsigned long *)p && *(unsigned long *)p <= value)
	{
		oldp = p;
		p += sizeof(long);	/* jump past long */
		while (*p++)		/* jump past string */
			if (p >= &symtable[SYMSIZE])
				break;
		ALIGN(p);
	}
	p = oldp + sizeof(long);        /* name string */
	
	cursym = (struct nlist *)(p - sizeof(long));
	return(value - cursym->n_value);
}

/*
 * lookupsym looks thru symtable to find the address of name.
 */

struct nlist *
lookup(name)
char *name;
{
	char *p;
	char *oldp;
	char *namep;
	int addus = 0;

again:
	p = symtable + sizeof(long);
	oldp = p;
	while (*p)
	{
		oldp = p;
		if (addus && *p == '_')
			p++;
		for (namep = name; *namep && *p; namep++, p++)
			if (*namep != *p)
				break;
		if (!*namep && !*p)
			return((struct nlist *)(oldp - sizeof(long)));
		while (*p++);		/* jump past rest of name */
		ALIGN(p);
		p += sizeof(long);	/* jump past long */
	}
	if (!addus) {
		addus = 1;
		goto again;
	}
	return((struct nlist *)0);
}

/*
 * Advance cursym to the next local variable.
 * Leave its value in localval as a side effect.
 * Return 0 at end of file.
 */
localsym(cframe, cargp)
long cframe, cargp;
{
	char *p;

	p = (char *)0;
	if (cursym) {
		p = (char *)cursym + sizeof(long);
		while (*p++)
			;
		ALIGN(p);
	}
	cursym = (struct nlist *)p;
	if (cursym && cursym->n_name[0] != '\0') {
		localval = cursym->n_value;
		return(1);
	}
	return(0);
}

/*
 * Print value v and then the string s.
 * If v is not zero, then we look for a nearby symbol
 * and print name+offset if we find a symbol for which
 * offset is small enough.
 *
 * For values which are just into kernel address space
 * that they match exactly or that they be more than maxoff
 * bytes into kernel space.
 */
psymoff(v, type, s)
long v;
int type;
char *s;
{
	unsigned long w;

	if (v)
		w = (unsigned long)findsym(v, type);
#if	1
	if (v==0 || w >= (unsigned long)maxoff)
#else	/* 1 */
		if (v==0 || w >= maxoff || (KVTOPH(v) < maxoff && w))
#endif	/* 1 */
			printf(LPRMODE, v);
		else {
#ifdef DDD
			printf("%s", cursym->n_un.n_name);
#else
			printf("%s", cursym->n_name);
#endif
			if (w)
				printf(OFFMODE, w);
		}
	printf(s);
}

/*
 * Print value v symbolically if it has a reasonable
 * interpretation as name+offset.  If not, print nothing.
 * Used in printing out registers $r.
 */
valpr(v, idsp)
long v;
{
	off_t d;

	d = findsym(v, idsp);
	if (d >= maxoff)
		return;
#ifdef DDD
	printf("%s", cursym->n_un.n_name);
#else
	printf("%s", cursym->n_name);
#endif
	if (d)
		printf(OFFMODE, d);
}
