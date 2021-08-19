/* 
 * Mach Operating System
 * Copyright (c) 1987 Carnegie-Mellon University
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 */

#ifndef		_KERN_MSG_
#define		_KERN_MSG_	1

/*
 * File:	kern_msg.h
 * Purpose:
 *	Kernel internal message structure.
 *
 * HISTORY
 * $Log:	kern_msg.h,v $
 * Revision 2.1  88/11/25  13:05:52  rvb
 * 2.1
 * 
 * Revision 2.6  88/10/11  10:23:58  rpd
 * 	Removed sender_task.
 * 	[88/10/06  07:51:01  rpd]
 * 	
 * 	Renamed "struct KMsg" as "struct kern_msg".  Renamed sender_to_notify
 * 	as sender_task, replaced sender_name with sender_entry.
 * 	[88/10/04  07:09:13  rpd]
 * 
 * Revision 2.5  88/08/24  02:30:20  mwyoung
 * 	Adjusted include file references.
 * 	[88/08/17  02:14:14  mwyoung]
 * 
 * Revision 2.4  88/08/06  19:20:28  rpd
 * Changed sys/mach_ipc_netport.h to kern/ipc_netport.h.
 * 
 * Revision 2.3  88/07/20  16:46:14  rpd
 * Added sender_name field.  It is used to generate msg-accepted notifications.
 * 
 * 17-Jan-88  Daniel Julin (dpj) at Carnegie-Mellon University
 *	MACH_NP: Added TCP control header.
 *
 * 30-Aug-87  Michael Young (mwyoung) at Carnegie-Mellon University
 *	MACH_NP: Conditionalize include of mch_ipc_vmtp.h.
 *
 * 23-Feb-87  Michael Young (mwyoung) at Carnegie-Mellon University
 *	Reduced kern_msg_t->action, kern_msg_t->sender to just one
 *	kern_msg_t->sender_to_notify field, a task.
 *	Remove kern_msg_action_t.
 *
 * 30-Jan-87  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Provide for proper compilation outside of the kernel.
 *
 *  9-Jan-87  Michael Young (mwyoung) at Carnegie-Mellon University
 *	Added #include's to make self-sufficient.
 *
 * 19-Jul-86  Michael Young (mwyoung) at Carnegie-Mellon University
 *	Revamped from "kern_ipc.h".
 */

#ifdef	KERNEL
#include <mach_np.h>
#else	/* KERNEL */
#include <sys/features.h>
#endif	/* KERNEL */

#include <sys/message.h>
#include <sys/task.h>
#include <sys/queue.h>
#include <sys/zalloc.h>

#if	MACH_NP
#include <kern/ipc_netport.h>
#endif	/* MACH_NP */

typedef struct kern_msg {
		queue_chain_t		queue_head;
		struct port_hash *	sender_entry;
		zone_t			home_zone;
#if	MACH_NP
		tcp_ctl_t		tcp_ctl;
		ipc_network_hdr_t	netmsg_hdr;
#endif	/* MACH_NP */
		msg_header_t		kmsg_header;
} *kern_msg_t;

#define		KERN_MSG_NULL	((kern_msg_t) 0)

#endif	/* 	_KERN_MSG_ */
