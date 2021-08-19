/* 
 * Mach Operating System
 * Copyright (c) 1987 Carnegie-Mellon University
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 */
/*
 **********************************************************************
 * HISTORY
 * 10-Oct-87  Robert V. Baron (rvb) at Carnegie-Mellon University
 *	Support for VAX650.
 *
 * 18-Aug-87  David Golub (dbg) at Carnegie-Mellon University
 *	Define cpu_number() as a macro returning 0 for single-cpu
 *	systems.
 *
 *  5-Aug-87  David Golub (dbg) at Carnegie-Mellon University
 *	Moved CPU_NUMBER_R0 to vax/asm.h.
 *
 *  2-May-87  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Support for VAX-8800.  Removed some gratuitous conditionals.
 *
 * 19-Oct-86  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Eliminated CS_NEWCPU.
 *
 * 25-Jun-86  Mike Accetta (mja) at Carnegie-Mellon University
 *	CS_GENERIC: reorganized ASSEMBLER and <sys/types.h> use.
 *	CS_NEWCPU:  updated for new processor specific support where
 *	still not provided in 4.3.
 *
 *  9-May-86  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Removed cpu_number_addr and CPU_NUMBER_DEST.  They are
 *	overridden by the cpu_number routine and CPU_NUMBER_R0 macros.
 *
 *  2-May-86  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Preliminary support for VAX 8200 (KA820).
 *
 **********************************************************************
 */
 
#ifdef	KERNEL
#include "cpus.h"
#include "cputypes.h"

#include "cs_generic.h"
#include "mach_mpm.h"
#else	/* KERNEL */
#include <sys/features.h>
#endif	/* KERNEL */
/*
 * Copyright (c) 1982, 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)cpu.h	7.1 (Berkeley) 6/5/86
 */
#ifndef	_CPU_
#define	_CPU_
#if	CS_GENERIC
 
/* 
 *  The ASSEMBLER flag subsumes LOCORE at CMU.
 */
#ifdef	ASSEMBLER
#ifdef	LOCORE
#define	LOCORE_	1
#else	/* LOCORE */
#define	LOCORE_	0
#endif	/* LOCORE */
#undef	LOCORE
#define	LOCORE	1
#endif	/* ASSEMBLER */
#endif	/* CS_GENERIC */

#ifndef LOCORE
#if	CS_GENERIC
/* 
 *  We use these types in the definitions below and hence ought to include them
 *  directly to aid new callers (even if our caller is likely to have already
 *  done so).
 */
#include <sys/types.h>

#endif	/* CS_GENERIC */
/*
 * Similar types are grouped with their earliest example.
 */
#define CPU_NextDimension	1

#ifdef KERNEL
int	cpu;
#if	MACH
int	master_cpu;
#if	NCPUS == 1
#define	cpu_number()	(0)
#define	set_cpu_number()
#endif	/* NCPUS == 1 */
#endif	/* MACH */
#endif
#endif
#if	CS_GENERIC
#ifdef	LOCORE_
#undef	LOCORE
#if	LOCORE_
#define	LOCORE	1
#endif	/* LOCORE_ */
#undef	LOCORE_
#endif	/* LOCORE_ */
#endif	/* CS_GENERIC */
#endif	/* _CPU_ */
