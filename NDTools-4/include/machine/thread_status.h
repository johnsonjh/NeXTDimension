/* 
 * Copyright (c) 1987, 1988 NeXT, Inc.
 */ 

#ifndef	_NeXT_THREAD_STATE_
#define	_NeXT_THREAD_STATE_

#import "next/reg.h"

/*
 *	NeXT_thread_state_regs	this is the structure that is exported
 *				to user threads for use in set/get status
 *				calls.  This structure should never
 *				change.
 *
 *	NeXT_thread_state_68882	this structure is exported to user threads
 *				to allow the to set/get 68882 floating
 *				pointer register state.
 *
 *	NeXT_saved_state	this structure corresponds to the state
 *				of the user registers as saved on the
 *				stack upon kernel entry.  This structure
 *				is used internally only.  Since this
 *				structure may change from version to
 *				version, it is hidden from the user.
 */

#define	NeXT_THREAD_STATE_REGS	(1)	/* normal registers */
#define NeXT_THREAD_STATE_68882	(2)	/* 68882 registers */
#define NeXT_THREAD_STATE_USER_REG (3)	/* additional user register */
#define	NextDimension_THREAD_STATE_REGS	(4)	/* normal registers */

struct NeXT_thread_state_regs {
	int	dreg[8];	/* data registers */
	int	areg[8];	/* address registers (incl stack pointer) */
	short	pad0;		/* not used */
	short	sr;		/* user's status register */
	int	pc;		/* user's program counter */
};

#define	NeXT_THREAD_STATE_REGS_COUNT \
	(sizeof (struct NeXT_thread_state_regs) / sizeof (int))

struct NeXT_thread_state_68882 {
	struct {
		int	fp[3];		/* 96-bit extended format */
	} regs[8];
	int	cr;			/* control */
	int	sr;			/* status */
	int	iar;			/* instruction address */
	int	state;			/* execution state */
};

#define	NeXT_THREAD_STATE_68882_COUNT \
	(sizeof (struct NeXT_thread_state_68882) / sizeof (int))

struct NeXT_thread_state_user_reg {
	int	user_reg;		/* user register (used by cthreads) */
};

#define NeXT_THREAD_STATE_USER_REG_COUNT \
	(sizeof (struct NeXT_thread_state_user_reg) / sizeof (int))

struct NextDimension_thread_state_regs {
	int	ireg[31];	/* core registers (incl stack pointer, but not r0) */
	int	freg[30];	/* FPU registers, except f0 and f1 */
	int	psr;		/* user's processor status register */
	int	epsr;		/* user's extended processor status register */
	int	db;		/* user's data breakpoint register */
	int	pc;		/* user's program counter */
	int	_padding_;	/* not used */
	/* Pipeline state for FPU */
	double	Mres3;
	double	Ares3;
	double	Mres2;
	double	Ares2;
	double	Mres1;
	double	Ares1;
	double	Ires1;
	double	Lres3m;
	double	Lres2m;
	double	Lres1m;
	double	KR;
	double	KI;
	double	T;
	int	Fsr3;
	int 	Fsr2;
	int	Fsr1;
	int	Mergelo32;
	int	Mergehi32;
};

#define	NextDimension_THREAD_STATE_REGS_COUNT \
	(sizeof (struct NextDimension_thread_state_regs) / sizeof (int))

#ifdef	KERNEL
struct NeXT_saved_state {
	struct	regs ss_r;
};

#define	USER_REGS(thread) \
	((thread)->pcb->saved_regs)

#endif	KERNEL

#endif	_NeXT_THREAD_STATE_
