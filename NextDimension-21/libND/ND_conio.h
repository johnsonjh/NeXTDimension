/*
 * This file contains the defs for the 'console' I/O interface used by ND kernel
 * printfs and the kdb debugger.  I wouldn't mess with this if I were you...
 */
#if ! defined(__ND_CONIO_H__)
#define __ND_CONIO_H__

#include "ND/ND_id.h"
#include <sys/message.h>

#define ND_CON_BUFSIZE	256
#define ND_CON_MASK	255

typedef struct ND_conio_s {
	msg_header_t	Head;
	int		Slot;
	int		Length;
	char		Data[ND_CON_BUFSIZE];
} ND_conio_t;

#define ND_CONIO_IDLE		-1

#define ND_CONIO_OUT		(ND_CONIO_START + 0)
#define ND_CONIO_IN_CHAR	(ND_CONIO_START + 1)
#define ND_CONIO_IN_STRING	(ND_CONIO_START + 2)
#define ND_CONIO_EXIT		(ND_CONIO_START + 4)
#define ND_CONIO_MESSAGE_AVAIL	(ND_CONIO_START + 5)
#define ND_CONIO_IN_STAND_BY	(ND_CONIO_START + 6)
#define ND_CONIO_MAX		ND_CONIO_IN_STAND_BY

#if defined(i860)
#if defined(PROTOTYPE)
#define ND_CONIO_MSG_ID ((volatile int *)(0xFFFFF800))
#define ND_CONIO_OUT_COUNT ((volatile int *)(0xFFFFF804))
#define ND_CONIO_OUT_BUF ((volatile char *)0xFFFFF808)
#define ND_CONIO_IN_COUNT ((volatile int *)(0xFFFFF908))
#define ND_CONIO_IN_BUF ((volatile char *)0xFFFFF90C)
#else /* ! PROTOTYPE */
#define ND_CONIO_MSG_ID ((volatile int *)(0xF80FF800))
#define ND_CONIO_OUT_COUNT ((volatile int *)(0xF80FF804))
#define ND_CONIO_OUT_BUF ((volatile char *)0xF80FF808)
#define ND_CONIO_IN_COUNT ((volatile int *)(0xF80FF908))
#define ND_CONIO_IN_BUF ((volatile char *)0xF80FF90C)
#endif /* ! PROTOTYPE */
#else
#if defined(KERNEL)
/*
 * In the kernel, all offsets are from the start of the globals rather than the
 * physical base address of the board.
 */
#define ND_CONIO_MSG_ID(handle) \
	*(int *)(((caddr_t)((handle)->globals_addr)) + 0xF800)
#define ND_CONIO_OUT_COUNT(handle) \
	*(int *)(((caddr_t)((handle)->globals_addr)) + 0xF804)
#define ND_CONIO_OUT_BUF(handle) \
	(((caddr_t)((handle)->globals_addr)) + 0xF808)
#define ND_CONIO_IN_COUNT(handle) \
	*(int *)(((caddr_t)((handle)->globals_addr)) + 0xF908)
#define ND_CONIO_IN_BUF(handle) \
	(((caddr_t)((handle)->globals_addr)) + 0xF90C)
#else /* ! KERNEL */
#if defined(PROTOTYPE)
#define ND_CONIO_MSG_ID(handle) \
	*(int *)(((caddr_t)((handle)->board_addr)) + 0x00FFF800)
#define ND_CONIO_OUT_COUNT(handle) \
	*(int *)(((caddr_t)((handle)->board_addr)) + 0x00FFF804)
#define ND_CONIO_OUT_BUF(handle) \
	(((caddr_t)((handle)->board_addr)) + 0x00FFF808)
#define ND_CONIO_IN_COUNT(handle) \
	*(int *)(((caddr_t)((handle)->board_addr)) + 0x00FFF908)
#define ND_CONIO_IN_BUF(handle) \
	(((caddr_t)((handle)->board_addr)) + 0x00FFF90C)
#else /* !PROTOTYPE */
#define ND_CONIO_MSG_ID(handle) \
	*(int *)(((caddr_t)((handle)->board_addr)) + 0x080FF800)
#define ND_CONIO_OUT_COUNT(handle) \
	*(int *)(((caddr_t)((handle)->board_addr)) + 0x080FF804)
#define ND_CONIO_OUT_BUF(handle) \
	(((caddr_t)((handle)->board_addr)) + 0x080FF808)
#define ND_CONIO_IN_COUNT(handle) \
	*(int *)(((caddr_t)((handle)->board_addr)) + 0x080FF908)
#define ND_CONIO_IN_BUF(handle) \
	(((caddr_t)((handle)->board_addr)) + 0x080FF90C)
#endif /* !PROTOTYPE */
#endif /* ! KERNEL */
#endif /* ! i860 */

#endif /* __ND_CONIO_H__ */

