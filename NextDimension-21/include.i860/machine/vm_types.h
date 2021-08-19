/* 
 * Mach Operating System
 * Copyright (c) 1987 Carnegie-Mellon University
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 */
/*
 *	File:	vm_types.h
 *	Author:	Avadis Tevanian, Jr.
 *
 *	Copyright (C) 1985, Avadis Tevanian, Jr.
 *
 *	Header file for VM data types.  i860 version.
 *
 * HISTORY
 * 23-Apr-87  Michael Young (mwyoung) at Carnegie-Mellon University
 *	Changed things to "unsigned int" to appease the user community :-).
 *
 * 13-Jun-85  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Created.
 */

#ifndef	_VM_TYPES_MACHINE_
#define	_VM_TYPES_MACHINE_	1

#ifdef	ASSEMBLER
#else	/* ASSEMBLER */
typedef	unsigned int	vm_offset_t;
typedef	unsigned int	vm_size_t;
#endif	/* ASSEMBLER */

#endif	/* _VM_TYPES_MACHINE_ */
