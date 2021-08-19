/* 
 * Mach Operating System
 * Copyright (c) 1987 Carnegie-Mellon University
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 */

/*
 * HISTORY
 *  9-Oct-87  Robert Baron (rvb) at Carnegie-Mellon University
 *	Define unix_reset for longjmp/setjmp reset.
 *
 * 21-Sep-87  Robert Baron (rvb) at Carnegie-Mellon University
 *	Created.
 *
 */

#include "cpus.h"

#if	NCPUS > 1

#define unix_master()  _unix_master()
#define unix_release() _unix_release()
#define unix_reset()   _unix_reset()
void _unix_master(), _unix_release(), _unix_reset();

#else	/* NCPUS > 1 */

#define unix_master()
#define unix_release()
#define unix_reset()

#endif	/* NCPUS > 1 */
