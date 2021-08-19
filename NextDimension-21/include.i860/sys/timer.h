/* 
 * Mach Operating System
 * Copyright (c) 1987 Carnegie-Mellon University
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 */
/*
 **********************************************************************
 * HISTORY
 * $Log:	timer.h,v $
 * Revision 2.1  88/11/25  13:07:11  rvb
 * 2.1
 * 
 * Revision 2.3  88/08/24  02:49:17  mwyoung
 * 	Adjusted include file references.
 * 	[88/08/17  02:25:43  mwyoung]
 * 
 *
 *  7-Apr-88  David Black (dlb) at Carnegie-Mellon University
 *	Added interface definitions and null routines for STAT_TIME.
 *
 * 19-Feb-88  David Black (dlb) at Carnegie-Mellon University
 *	Renamed fields, added timer_save structure.
 *
 * 31-May-87  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	machine/machtimer.h -> machine/timer.h
 *
 * 23-Feb-87  David L. Black (dlb) at Carnegie-Mellon University
 *	Created.
 **********************************************************************
 */ 

#ifndef	_TIMER_
#define _TIMER_

#include <stat_time.h>

#include <cpus.h>

#if	STAT_TIME
/*
 *	Statistical timer definitions - use microseconds in timer, seconds
 *	in high unit field.  No adjustment needed to convert to time_value_t
 *	as a result.  Service timers once an hour.
 */

#define	TIMER_RATE	1000000
#define TIMER_HIGH_UNIT	TIMER_RATE
#undef	TIMER_ADJUST
#define	TIMER_SERVICE_PERIOD	(3600*hz)

#else	/* STAT_TIME */
/*
 *	Machine dependent definitions based on hardware support.
 */

#include <machine/timer.h>

#endif	/* STAT_TIME */

/*
 *	Definitions for accurate timers.
 */

struct timer {
	unsigned	low_bits;
	unsigned	high_bits;
	unsigned	tstamp;
	unsigned	flags;
};

typedef struct timer		timer_data_t;
typedef	struct timer		*timer_t;

/*
 *	Flags for flags field.
 */

#define	TIMER_STATE	1
#define	TIMER_SERVICE	2
#define	TIMER_BUSY	4

/*
 *	Kernel timers and current timer array.  [Exported]
 */

timer_t		current_timer[NCPUS];
timer_data_t	kernel_timer[NCPUS];

/*
 *	save structure for timer readings.  This is used to save timer
 *	readings for elapsed time computations.
 */

struct timer_save {
	unsigned	low;
	unsigned	high;
};

typedef struct timer_save	timer_save_data_t, *timer_save_t;

/*
 *	Exported kernel interface to timers
 */

#if	STAT_TIME
#define start_timer(timer)
#define	timer_switch(timer)
#else	/* STAT_TIME */
void	start_timer();
void	timer_switch();
#endif	/* STAT_TIME */

void		timer_read();
void		thread_read_times();
unsigned	timer_delta();

#if	STAT_TIME
/*
 *	Macro to bump timer values.
 */	
#define timer_bump(timer, usec)	{				\
	(timer)->low_bits += usec;				\
	if ((timer)->flags & TIMER_SERVICE) {			\
		timer_normalize(timer);				\
	}							\
}

#else	/* STAT_TIME */
/*
 *	Exported hardware interface to timers
 */
void	time_trap_uentry();
void	time_trap_uexit();
timer_t	time_int_entry();
void	time_int_exit();
#endif	/* STAT_TIME */

/*
 *	TIMER_DELTA finds the difference between a timer and a saved value,
 *	and updates the saved value.
 */

#define	TIMER_DELTA(timer, save, result) {			\
	register unsigned	temp;				\
								\
	temp = (timer).low_bits;				\
	if ((save).high != (timer).high_bits) {			\
		result += timer_delta(&(timer), &(save));	\
	}							\
	else {							\
		result += temp - (save).low;			\
		(save).low = temp;				\
	}							\
}

#endif	/* _TIMER_ */
