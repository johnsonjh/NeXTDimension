/*
 * Copyright 1982 by Michael J. Paquette
 *
 * Project:	The Retargetable C compiler
 * Package:	libc library
 */

/*
 * Compare strings:  s1>s2: >0  s1==s2: 0  s1<s2: <0
 */

strcmp(s1, s2)
char *s1, *s2;
{

	while (*s1 == *s2++)
		if (*s1++=='\0')
			return(0);
	return(*s1 - *--s2);
}
