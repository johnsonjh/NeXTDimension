/*
 * Definitions for message passing queues between i860 and the host processor.
 *
 * Items bracketed by #if defined(i860) are specific to the i860 implementation.
 */
#if ! defined(__ND_MSG_H__)
#define __ND_MSG_H__

#include <sys/port.h>		/* for def of port_t */

#define MSGBUF_SIZE			0x10000
#define MSGBUF_MASK			(MSGBUF_SIZE - 1)

/* Bits set in the NDMsgQueues Flags field to enable interrupt messages to the host */
#define MSGFLAG_MSG_OUT_AVAIL		1	/* 860 to host queue no longer empty */
#define MSGFLAG_MSG_IN_EMPTY		2	/* host to 860 queue is now empty */
#define MSGFLAG_MSG_IN_LOW		4	/* host to 860 queue has free space */
#define MSGFLAG_MSG_REPLY_EMPTY		8	/* host to 860 reply queue empty */
#define MSGFLAG_MSG_REPLY_LOW		0x10	/* host to 860 reply queue free space*/
#define MSGFLAG_MSG_SYS_READY		0x80000000 /* Message sys ready (set by ND) */

/*
 * Lamport lock keys used to interlock the ND queues between DPS, kernel,
 * and access from other bus master boards.
 */
#if defined(i860)
unsigned long           _slot_id_;
#define LOCK_NUM	(_slot_id_ >> 28)
#else
#if defined(KERNEL)
#define LOCK_NUM	1
#else
#define LOCK_NUM	0
#endif
#endif

#define LOCK_UNDEF	-1
#define LOCK_SLOT2	2
#define LOCK_SLOT4	4
#define LOCK_SLOT6	6
#define MAX_LOCK_NUM	LOCK_SLOT6

typedef struct _NDQueue_
{
	volatile char	*Buf;
	volatile int	Head;
	volatile int	Tail;
	volatile int	Lock_x;			/* Lamport lock registers for queue */
	volatile int	Lock_y;
	volatile int	Lock_b[MAX_LOCK_NUM+1];
} NDQueue;

typedef struct _NDMsgQueues_
{
	NDQueue	ToND;
	NDQueue	FromND;
	NDQueue	ReplyND;
	unsigned Flags;
	port_t	kernel_port;			/* ND receives on this port */
	port_t	debug_port;			/* ND sends on this port */
	port_t	service_port;			/* ND sends on this port */
	port_t	service_replyport;		/* ND receives on this port */
	port_t	pager_port;			/* ND sends to this port. */
	port_t	pager_reply;			/* ND pager recieves on this port. */
} NDMsgQueues;

#if defined(PROTOTYPE)

#if defined(i860)
#define ND_START_UNCACHEABLE_DRAM	0xFFFC0000
#define MSGBUF_860_TO_HOST	((char *)ND_START_UNCACHEABLE_DRAM)
#define MSGBUF_HOST_TO_860	((char *)(ND_START_UNCACHEABLE_DRAM + MSGBUF_SIZE))
#define MSGBUF_REPLY_TO_860	((char *)(ND_START_UNCACHEABLE_DRAM + MSGBUF_SIZE*2))
#define MSG_QUEUES	((NDMsgQueues *)(ND_START_UNCACHEABLE_DRAM + MSGBUF_SIZE*3))
#else
#if defined(KERNEL)
/*
 * In the kernel, the board_addr field may only be used safely in the service threads,
 * where transparent translation is active.
 */
#define MSGBUF_860_TO_HOST(ND_var) \
		(caddr_t)(((char *)((ND_var)->board_addr))+0xFC0000)
#define MSGBUF_HOST_TO_860(ND_var) \
		(caddr_t)(((char *)((ND_var)->board_addr))+0xFD0000)
#define MSGBUF_REPLY_TO_860(ND_var) \
		(caddr_t)(((char *)((ND_var)->board_addr))+0xFE0000)
#define MSG_QUEUES(ND_var) \
		((NDMsgQueues *)(((char *)((ND_var)->globals_addr))))
#else /* ! KERNEL */
/*
 * WARNING: dram_addr is invalid most of the time on the host. Don't count on it!
 */
#define MSGBUF_860_TO_HOST(ND_var) \
		(caddr_t)(((char *)((ND_var)->dram_addr))+0x7C0000)
#define MSGBUF_HOST_TO_860(ND_var) \
		(caddr_t)(((char *)((ND_var)->host_to_ND_addr)))
#define MSG_QUEUES(ND_var) \
		((NDMsgQueues *)(((char *)((ND_var)->globals_addr))))
#endif /* ! KERNEL */
#endif /* ! i860 */

#else /* !PROTOTYPE */

#if defined(i860)
#define ND_START_UNCACHEABLE_DRAM	0xF80C0000
#define MSGBUF_860_TO_HOST	((char *)ND_START_UNCACHEABLE_DRAM)
#define MSGBUF_HOST_TO_860	((char *)(ND_START_UNCACHEABLE_DRAM + MSGBUF_SIZE))
#define MSGBUF_REPLY_TO_860	((char *)(ND_START_UNCACHEABLE_DRAM + MSGBUF_SIZE*2))
#define MSG_QUEUES	((NDMsgQueues *)(ND_START_UNCACHEABLE_DRAM + MSGBUF_SIZE*3))
#define ND_END_UNCACHEABLE_DRAM	0xF8100000
#else
#if defined(KERNEL)
/*
 * In the kernel, the board_addr field may only be used safely in the service threads,
 * where transparent translation is active.
 */
#define MSGBUF_860_TO_HOST(ND_var) \
		(caddr_t)(((char *)((ND_var)->board_addr))+0x080C0000)
#define MSGBUF_HOST_TO_860(ND_var) \
		(caddr_t)(((char *)((ND_var)->board_addr))+0x080D0000)
#define MSGBUF_REPLY_TO_860(ND_var) \
		(caddr_t)(((char *)((ND_var)->board_addr))+0x080E0000)
#define MSG_QUEUES(ND_var) \
		((NDMsgQueues *)(((char *)((ND_var)->globals_addr))))
#else /* ! KERNEL */
/*
 * WARNING: dram_addr is invalid most of the time on the host. Don't count on it!
 */
#define MSGBUF_860_TO_HOST(ND_var) \
		(caddr_t)(((char *)((ND_var)->dram_addr))+0x000C0000)
#define MSGBUF_HOST_TO_860(ND_var) \
		(caddr_t)(((char *)((ND_var)->host_to_ND_addr)))
#define MSG_QUEUES(ND_var) \
		((NDMsgQueues *)(((char *)((ND_var)->globals_addr))))
#endif /* ! KERNEL */
#endif /* ! i860 */
#endif /* !PROTOTYPE */

#define BYTES_ON_QUEUE(q)	(((q)->Head - (q)->Tail) & MSGBUF_MASK)

#endif


