/*	Copyright (c) 1984 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 * MACH addendum
 *
 * 11-Jul-88	John Dugas, Scott Nacey  Prime Computer Inc.
 * 	Added pcb structure
 */

#ident	"@(#)head.sys:tss.h	1.4"

#ifndef _PCB_
#define _PCB_

#include "sys/types.h"

/*
 * i860 PCB definition
 */

struct pcb {
	struct pt_entry *pcb_direct;	/* page directory pointer */
	label_t	pcb_context;		/* save context for kernel */
	int	pcb_cmap2;
	int	*pcb_sswap;
	int	pcb_sigc[5];
};

/*
 *	pcb_synch is not needed on the i860
 */

#define pcb_synch(x)
#endif
