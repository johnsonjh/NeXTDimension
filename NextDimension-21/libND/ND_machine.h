/* 
 * Mach Operating System
 * Copyright (c) 1987 Carnegie-Mellon University
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 */
/*
 *	File:	h/machine.h
 *	Author:	Avadis Tevanian, Jr.
 *
 *	Copyright (C) 1986, Avadis Tevanian, Jr.
 *
 *	Machine independent machine abstraction.
 *
 ************************************************************************
 * HISTORY
 *
 * 16-Aug-89  Mike Paquette (mpaque) at NeXT
 *	Added I860 CPU type and subtypes for big or little-endian data implementation.
 *
 * 18-Nov-87  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Define UVAXIII.
 *
 * 27-Jul-87  Bill Bolosky (bolosky) at Carnegie-Mellon University
 *	Added CPU_SUBTYPE_RT_APC for PC/RT Advanced Processor Card (CMOS
 *	romp).
 *
 * 23-Jun-87  David Black (dlb) at Carnegie-Mellon University
 *	Assume that NEW_SCHED is enabled.
 *	MACH_TT: tick_count no longer needed.
 *
 * 15-May-87  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	features.h not necessary for non-KERNEL compiles.
 *
 * 29-Apr-87  David Kirschen (kirschen) at Encore Computer Corporation
 *      Added NS32332
 *
 * 25-Apr-87  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Define a field for clock frequency in the machine_slot structure.
 *	This is in the slot structure on the off chance that we may
 *	someday have different clock rates on different processors.
 *
 * 10-Apr-87  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Guard against multiple file inclusion.
 *
 * 25-Mar-87  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Added major and minor version numbers to the machine_info
 *	structure.
 *
 * 18-Mar-87  John Seamons (jks) at NeXT
 *	Added CPU_TYPE_MC68030 and CPU_SUBTYPE_NeXT.
 *
 * 28-Feb-87  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Separate Sun-3 types into 3/50, 3/160, and 3/260.
 *
 * 28-Feb-87  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Define machine_{info,slot}_data_t types for MiG.  Yes, MiG is
 *	bogus, but I don't have the time to fix it.
 *
 * 31-Jan-87  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Define cpu_ticks arrays in machine slots.
 *
 * 19-Jan-87  David L. Black (dlb) at Carnegie-Mellon University
 *	NEW_SCHED: cpu_idle removed; replaced by macro in sched.h.
 *
 * 08-Jan-87  Robert Beck (beck) at Sequent Computer Systems, Inc.
 *	Add CPU_SUBTYPE_SQT.
 *
 * 30-Oct-86  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Add cpu_idle array.
 *
 * 24-Sep-86  Michael Young (mwyoung) at Carnegie-Mellon University
 *	Changed to directly import boolean declaration.
 *
 * 16-May-86  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Created.
 *
 ************************************************************************
 */

#ifndef	_SYS_MACHINE_H_
#define _SYS_MACHINE_H_
#define _MACHINE_	/* Backwards compatability */

#if	defined(KERNEL) && !defined(KERNEL_FEATURES)
#include "cpus.h"
#else	defined(KERNEL) && !defined(KERNEL_FEATURES)
#include <sys/features.h>
#endif	defined(KERNEL) && !defined(KERNEL_FEATURES)

#include <vm/vm_param.h>
#include <sys/boolean.h>

/*
 *	For each host, there is a maximum possible number of
 *	cpus that may be available in the system.  This is the
 *	compile-time constant NCPUS, which is defined in cpus.h.
 *
 *	In addition, there is a machine_slot specifier for each
 *	possible cpu in the system.
 */

struct machine_info {
	int		major_version;	/* kernel major version id */
	int		minor_version;	/* kernel minor version id */
	int		max_cpus;	/* max number of cpus compiled */
	int		avail_cpus;	/* number actually available */
	vm_size_t	memory_size;	/* size of memory in bytes */
};

typedef struct machine_info	*machine_info_t;
typedef struct machine_info	machine_info_data_t;	/* bogus */

typedef int	cpu_type_t;
typedef int	cpu_subtype_t;

#define	CPU_STATE_MAX		3

#define	CPU_STATE_USER		0
#define	CPU_STATE_SYSTEM	1
#define	CPU_STATE_IDLE		2

struct machine_slot {
	boolean_t	is_cpu;		/* is there a cpu in this slot? */
	cpu_type_t	cpu_type;	/* type of cpu */
	cpu_subtype_t	cpu_subtype;	/* subtype of cpu */
	boolean_t	running;	/* is cpu running */
	long		cpu_ticks[CPU_STATE_MAX];
	int		clock_freq;	/* clock interrupt frequency */
};

typedef struct machine_slot	*machine_slot_t;
typedef struct machine_slot	machine_slot_data_t;	/* bogus */

#ifdef	KERNEL
struct machine_info		machine_info;
struct machine_slot		machine_slot[NCPUS];

boolean_t	should_exit[NCPUS];
vm_offset_t	interrupt_stack[NCPUS];
#endif	KERNEL

/*
 *	Machine types known by all.
 */

#define	CPU_TYPE_VAX		((cpu_type_t) 1)
#define	CPU_TYPE_ROMP		((cpu_type_t) 2)
#define	CPU_TYPE_MC68020	((cpu_type_t) 3)
#define CPU_TYPE_NS32032	((cpu_type_t) 4)
#define CPU_TYPE_NS32332        ((cpu_type_t) 5)
#define CPU_TYPE_MC68030	((cpu_type_t) 6)
#define CPU_TYPE_I860		((cpu_type_t) 7)
/*
 *	Machine subtypes (these are defined here, instead of in a machine
 *	dependent directory, so that any program can get all definitions
 *	regardless of where is it compiled).
 */

/*
 *	VAX subtypes (these do *not* necessary conform to the actual cpu
 *	ID assigned by DEC available via the SID register).
 */

#define CPU_SUBTYPE_VAX780	((cpu_subtype_t) 1)
#define CPU_SUBTYPE_VAX785	((cpu_subtype_t) 2)
#define CPU_SUBTYPE_VAX750	((cpu_subtype_t) 3)
#define CPU_SUBTYPE_VAX730	((cpu_subtype_t) 4)
#define CPU_SUBTYPE_UVAXI	((cpu_subtype_t) 5)
#define CPU_SUBTYPE_UVAXII	((cpu_subtype_t) 6)
#define CPU_SUBTYPE_VAX8200	((cpu_subtype_t) 7)
#define CPU_SUBTYPE_VAX8500	((cpu_subtype_t) 8)
#define CPU_SUBTYPE_VAX8600	((cpu_subtype_t) 9)
#define CPU_SUBTYPE_VAX8650	((cpu_subtype_t) 10)
#define CPU_SUBTYPE_VAX8800	((cpu_subtype_t) 11)
#define CPU_SUBTYPE_UVAXIII	((cpu_subtype_t) 12)

/*
 *	ROMP subtypes.
 */

#define CPU_SUBTYPE_RT_PC	((cpu_subtype_t) 1)
#define	CPU_SUBTYPE_RT_APC	((cpu_subtype_t) 2)

/*
 *	68020 subtypes.
 */

#define CPU_SUBTYPE_SUN3_50	((cpu_subtype_t) 1)
#define CPU_SUBTYPE_SUN3_160	((cpu_subtype_t) 2)
#define CPU_SUBTYPE_SUN3_260	((cpu_subtype_t) 3)

/*
 *	32032/32332 subtypes.
 */

#define CPU_SUBTYPE_MMAX	((cpu_subtype_t) 1)
#define CPU_SUBTYPE_SQT		((cpu_subtype_t) 2)

/*
 *	68030 subtypes.
 */

#define CPU_SUBTYPE_NeXT	((cpu_subtype_t) 1)

/*
 *	I860 subtypes
 */
#define CPU_SUBTYPE_LITTLE_ENDIAN	((cpu_subtype_t) 0)
#define CPU_SUBTYPE_BIG_ENDIAN		((cpu_subtype_t) 1)

#endif	_SYS_MACHINE_H_

