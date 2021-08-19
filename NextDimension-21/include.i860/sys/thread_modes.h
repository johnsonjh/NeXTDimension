/* 
 * Mach Operating System
 * Copyright (c) 1987 Carnegie-Mellon University
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 */
/*
 **********************************************************************
 * HISTORY
 *  3-Apr-87  David Black (dlb) at Carnegie-Mellon University
 *	Created by extracting thread mode defines from thread.h.
 *
 **********************************************************************
 */ 

/*
 *	Maximum number of thread execution modes.
 */

#define	THREAD_MAXMODES		2

/*
 *	Thread execution modes.
 */

#define	THREAD_USERMODE		0
#define	THREAD_SYSTEMMODE	1
