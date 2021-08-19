/*
 ****************************************************************
 * Mach Operating System
 * Copyright (c) 1986 Carnegie-Mellon University
 *  
 * This software was developed by the Mach operating system
 * project at Carnegie-Mellon University's Department of Computer
 * Science. Software contributors as of May 1986 include Mike Accetta, 
 * Robert Baron, William Bolosky, Jonathan Chew, David Golub, 
 * Glenn Marcy, Richard Rashid, Avie Tevanian and Michael Young. 
 * 
 * Some software in these files are derived from sources other
 * than CMU.  Previous copyright and other source notices are
 * preserved below and permission to use such software is
 * dependent on licenses from those institutions.
 * 
 * Permission to use the CMU portion of this software for 
 * any non-commercial research and development purpose is
 * granted with the understanding that appropriate credit
 * will be given to CMU, the Mach project and its authors.
 * The Mach project would appreciate being notified of any
 * modifications and of redistribution of this software so that
 * bug fixes and enhancements may be distributed to users.
 *
 * All other rights are reserved to Carnegie-Mellon University.
 ****************************************************************
 */
/*
 **************************************************************************
 * HISTORY
 * 11-Oct-85  Robert V Baron (rvb) at Carnegie-Mellon University
 *	header for the "n" stack trace.  Used to describe a stack frame.
 *
 **************************************************************************
 */

struct frame {
	int *f_handler;
	struct f_fm {
		unsigned int : 5;
		unsigned int fm_psw : 11;
		unsigned int fm_mask : 12;
		unsigned int : 1;
		unsigned int fm_s : 1;
		unsigned int fm_spa : 2;
	} f_msk ;
	int *f_ap ;
	struct frame *f_fp ;
	unsigned char *f_pc ;
	int f_r0[12] ;
} ;

#define f_psw f_msk.f_fmask
#define f_mask f_msk.fm_mask
#define f_s f_msk.fm_s
#define f_spa f_msk.fm_spa
