
/* 
 * Mach Operating System
 * Copyright (c) 1987 Carnegie-Mellon University
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 */
/*
 *	File:	sched.h
 *	Author:	Avadis Tevanian, Jr.
 *
 *	Copyright (C) 1985, Avadis Tevanian, Jr.
 *
 *	Header file for scheduler.
 *
 * HISTORY
 * $Log:	sched.h,v $
 * Revision 2.1  88/11/25  13:06:37  rvb
 * 2.1
 * 
 * Revision 2.3  88/08/24  02:42:44  mwyoung
 * 	Adjusted include file references.
 * 	[88/08/17  02:22:04  mwyoung]
 * 
 *
 * 29-Mar-88  David Black (dlb) at Carnegie-Mellon University
 *	SIMPLE_CLOCK: added sched_usec for drift compensation.
 *
 * 25-Mar-88  David Black (dlb) at Carnegie-Mellon University
 *	Added sched_load and related constants.  Moved thread_timer_delta
 *	here because it depends on sched_load.
 *
 * 19-Feb-88  David Black (dlb) at Carnegie-Mellon University
 *	Added sched_tick and shift definitions for more flexible ageing.
 *
 * 18-Nov-87  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Removed conditionals, purged history.
 */

#ifndef	_SCHED_
#define	_SCHED_

#include <cpus.h>
#include <simple_clock.h>
#include <stat_time.h>

#include <sys/queue.h>
#include <sys/lock.h>

#if	STAT_TIME

/*
 *	Statistical timing uses microseconds as timer units.  16 bit shift
 *	yields priorities.  PRI_SHIFT_2 isn't needed.
 */
#define	PRI_SHIFT	16

#else	/* STAT_TIME */

/*
 *	Otherwise machine provides shift(s) based on time units it uses.
 */
#include <machine/sched_param.h>

#endif	/* STAT_TIME */

#define NRQS	32			/* 32 run queues per cpu */

struct run_queue {
	queue_head_t		runq[NRQS];	/* one for each priority */
	simple_lock_data_t	lock;		/* one lock for all queues */
	int			low;		/* low queue value */
	int			count;		/* count of threads runable */
};

typedef struct run_queue	*run_queue_t;
#define	RUN_QUEUE_NULL	((run_queue_t) 0)

#ifdef	KERNEL
struct run_queue	global_runq;
struct run_queue	local_runq[NCPUS];
#endif	/* KERNEL */

#define other_threads_runnable(rqi, runq) \
	(((runq)->count > 0) &&  (rqi >= (runq)->low))

/*
 *	Scheduler routines.
 */

struct run_queue	*rem_runq();
struct thread		*choose_thread();

/*
 *	Structures for efficient dispatching of idle cpus.
 */

struct cpu_info {
	struct queue_entry	cpi_queue;	/* for idle_queue */
	int			cpi_state;	/* state of each cpu */
	int			cpi_number;	/* this cpu's number */
};

typedef	struct cpu_info		*cpu_info_t;

#define	cpu_state(cpu)		cpu_info[cpu].cpi_state

#define CPU_OFF_LINE	(-1)
#define	CPU_RUNNING	0
#define	CPU_IDLE	1
#define CPU_DISPATCHING	2

#define	cpu_idle(cpu)	(cpu_info[cpu].cpi_state == CPU_IDLE)

#ifdef	KERNEL
struct cpu_info cpu_info[NCPUS];
struct thread	*next_thread[NCPUS];	/* next thread to run */
int		runrun[NCPUS];		/* cpu can be resched. at same pri */

struct thread	*idle_thread_array[NCPUS];	/* holds idle threads */

queue_head_t	idle_queue;		/* queue for idle cpus */
int		idle_count;		/* number of idle cpus */
simple_lock_data_t	idle_lock;	/* lock for idle structures */

	/*
	 *	Data structures for runtime quantum adjustment.
	 */
#if	NCPUS > 1
int			machine_quantum[NCPUS+1];
int			quantum_adj_index;	/* used to spread out cpus
						   when quantum changes */
simple_lock_data_t	quantum_adj_lock;

int			last_quantum[NCPUS];	/* used to detect changes */
#endif	/* NCPUS > 1 */
int			min_quantum;	/* defines max context switch rate */
int			sys_quantum;	/* used by set_runq */
#endif	/* KERNEL */

/*
 *	Default base priorities for threads.
 */
#define BASEPRI_SYSTEM	25
#define BASEPRI_USER	50

/*
 *	Shift structures for holding update shifts.  Actual computation
 *	is  usage = (usage >> shift1) +/- (usage >> abs(shift2))  where the
 *	+/- is determined by the sign of shift 2.
 */
struct shift {
	int	shift1;
	int	shift2;
};

typedef	struct shift	*shift_t, shift_data_t;

/*
 *	sched_tick increments once a second.  Used to age priorities.
 *	sched_load is the load factor for the scheduler.
 */

unsigned	sched_tick;
int		sched_load;

#define	SCHED_SCALE	128
#define SCHED_SHIFT	7

/*
 *	thread_timer_delta macro takes care of both thread timers.
 */
 
#define thread_timer_delta(thread) do { 			\
	register unsigned	delta;				\
								\
	delta = 0;						\
	TIMER_DELTA((thread)->system_timer,			\
		(thread)->system_timer_save, delta)		\
	TIMER_DELTA((thread)->user_timer,			\
		(thread)->user_timer_save, delta)		\
	(thread)->cpu_delta += delta;				\
	(thread)->sched_delta += delta * sched_load;		\
} while(FALSE)

#if	SIMPLE_CLOCK
/*
 *	sched_usec is an exponential average of number of microseconds
 *	in a second for clock drift compensation.
 */

int	sched_usec;

#endif /* SIMPLE_CLOCK */

#endif	/* _SCHED_ */
