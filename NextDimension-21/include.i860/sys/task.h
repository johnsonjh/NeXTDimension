/* 
 * Mach Operating System
 * Copyright (c) 1988 Carnegie-Mellon University
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 */
/*
 *	File:	task.h
 *	Author:	Avadis Tevanian, Jr.
 *
 *	This file contains the structure definitions for tasks.
 *
 * HISTORY
 * $Log:	task.h,v $
 * Revision 2.1  88/11/25  13:06:54  rvb
 * 2.1
 * 
 * Revision 2.6  88/09/25  22:16:41  rpd
 * 	Changed port_cache fields/definitions to obj_cache.
 * 	[88/09/24  18:13:13  rpd]
 * 
 * Revision 2.5  88/08/24  02:46:30  mwyoung
 * 	Adjusted include file references.
 * 	[88/08/17  02:24:13  mwyoung]
 * 
 * Revision 2.4  88/07/20  21:07:49  rpd
 * Added ipc_task_lock/ipc_task_unlock definitions.
 * Changes for port sets.
 * Add ipc_next_name field, used for assigning local port names.
 * 
 * Revision 2.3  88/07/17  18:56:33  mwyoung
 * .
 * 
 * Revision 2.2.2.1  88/06/28  20:02:03  mwyoung
 * Cleaned up.  Replaced task_t->kernel_only with
 * task_t->kernel_ipc_space, task_t->kernel_vm_space, and
 * task_t->ipc_privilege, to prevent overloading errors.
 * 
 * Remove current_task() declaration.
 * Eliminate paging_task.
 * 
 * Revision 2.2.1.2  88/06/26  00:45:49  rpd
 * Changes for port sets.
 * 
 * Revision 2.2.1.1  88/06/23  23:32:38  rpd
 * Add ipc_next_name field, used for assigning local port names.
 * 
 * 21-Jun-88  Michael Young (mwyoung) at Carnegie-Mellon University
 *	Cleaned up.  Replaced task_t->kernel_only with
 *	task_t->kernel_ipc_space, task_t->kernel_vm_space, and
 *	task_t->ipc_privilege, to prevent overloading errors.
 *
 * 19-Apr-88  Michael Young (mwyoung) at Carnegie-Mellon University
 *	Remove current_task() declaration.
 *	Eliminate paging_task.
 *
 * 18-Jan-88  David Golub (dbg) at Carnegie-Mellon University
 *	Removed task_data (now is per_thread).  Added
 *	task_bootstrap_port.  Added new routine declarations.
 *	Removed wake_active (unused).  Added fields to accumulate
 *	user and system time for terminated threads.
 *
 *  19-Feb-88 Douglas Orr (dorr) at Carnegie-Mellon University
 *	Change emulation bit mask into vector of routine  addrs
 *
 *  27-Jan-87 Douglas Orr (dorr) at Carnegie-Mellon University
 *	Add support for user space syscall emulation (bit mask
 *	of enabled user space syscalls and user space emulation
 *	routine).
 *
 *  3-Dec-87  Michael Young (mwyoung) at Carnegie-Mellon University
 *	Change port cache account for per-task port names.
 *	Should move IPC stuff to a separate file :-).
 *	Add reply port for use by kernel-internal tasks.
 *
 *  2-Dec-87  David Black (dlb) at Carnegie-Mellon University
 *	Added active field.
 *
 * 18-Nov-87  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Eliminate conditionals, flush history.
 */

#ifndef	_TASK_
#define	_TASK_

/* Users really ought not be including this file. */
#ifdef	KERNEL
#include <mch_emulatn.h>
#else	/* KERNEL */
#include <sys/features.h>
#endif	/* KERNEL */

#include <sys/lock.h>
#include <sys/queue.h>
#include <vm/vm_map.h>
#include <sys/port.h>
#include <sys/time_value.h>
#include <sys/mach_param.h>
#include <sys/boolean.h>
#include <sys/kern_obj.h>
#include <sys/kern_set.h>
#if	MACH_EMULATION
#include <sys/syscall_emulation.h>
#endif	/* MACH_EMULATION */

struct task {
	/* Synchronization/destruction information */
	simple_lock_data_t lock;	/* Task's lock */
	int		ref_count;	/* Number of references to me */
	boolean_t	active;		/* Task has not been terminated */

	/* Miscellaneous */
	vm_map_t	map;		/* Address space description */
	queue_chain_t	all_tasks;	/* list of all tasks */
	int		suspend_count;	/* Internal scheduling only */

	/* Thread information */
	queue_head_t	thread_list;	/* list of threads */
	int		thread_count;	/* number of threads */

	/* Garbage */
	struct utask	*u_address;
	int		proc_index;	/* corresponding process, by index */

	/* User-visible scheduling information */
	int		user_suspend_count;	/* outstanding suspends */
	int		user_stop_count;	/* outstanding stops */

	/* Information for kernel-internal tasks */
	boolean_t	kernel_ipc_space; /* Uses kernel's port names? */
	boolean_t	kernel_vm_space; /* Uses kernel's pmap? */

	/* Statistics */
	time_value_t	total_user_time;
				/* total user time for dead threads */
	time_value_t	total_system_time;
				/* total system time for dead threads */

	/* Special ports */
	port_t		task_self;	/* Port representing the task */
	port_t		task_notify;	/* Where notifications get sent */
	port_t		exception_port;	/* Where exceptions are sent */
	port_t		bootstrap_port;	/* Port passed on for task startup */

	port_t		reply_port;	/* Used by internal tasks (MiG) */

	/* IPC structures */
	boolean_t	ipc_privilege;	/* Can use kernel resource pools? */
	simple_lock_data_t ipc_translation_lock;
	queue_head_t	ipc_translations; /* Per-task port naming */
	boolean_t	ipc_active;	/* Can IPC rights be added? */
	port_name_t	ipc_next_name;	/* Next local name to use */
	kern_set_t	ipc_enabled;	/* Port set for PORT_ENABLED */

#define	OBJ_CACHE_LOCAL		0
#define	OBJ_CACHE_REMOTE	1
#define	OBJ_CACHE_MAX		2
	struct {
		port_name_t	name;
		kern_obj_t	object;
	}		obj_cache[OBJ_CACHE_MAX];
					/* Fast object translation cache */

	/* IPC compatibility garbage */
	boolean_t	ipc_intr_msg;	/* Send signal upon message arrival? */
	port_t		ipc_ports_registered[TASK_PORT_REGISTER_MAX];

	/* User space system call emulation support */
#if	MACH_EMULATION	
	struct 	eml_dispatch	*eml_dispatch;
#endif	/* MACH_EMULATION		 */
};

typedef struct task *task_t;
#define TASK_NULL	((task_t) 0)

#ifdef	KERNEL
queue_head_t all_tasks; /* list of all tasks in the system */
simple_lock_data_t all_tasks_lock; /* lock for task list */

#define	task_lock(task)		simple_lock(&(task)->lock)
#define	task_unlock(task)	simple_unlock(&(task)->lock)

#define	ipc_task_lock(t)	simple_lock(&(t)->ipc_translation_lock)
#define	ipc_task_unlock(t)	simple_unlock(&(t)->ipc_translation_lock)
#endif	/* KERNEL */

/*
 *	Exported routines/macros
 */

kern_return_t	task_create();
kern_return_t	task_terminate();
kern_return_t	task_suspend();
kern_return_t	task_resume();
kern_return_t	task_threads();
kern_return_t	task_ports();
kern_return_t	task_info();
kern_return_t	task_get_special_port();
kern_return_t	task_set_special_port();

#ifdef	KERNEL
/*
 *	Internal only routines
 */

void		task_init();
void		task_reference();
void		task_deallocate();
kern_return_t	task_hold();
kern_return_t	task_dowait();
kern_return_t	task_release();
kern_return_t	task_halt();

kern_return_t	task_suspend_nowait();

extern task_t	kernel_task;
#endif	/* KERNEL */

#endif	/* _TASK_ */
