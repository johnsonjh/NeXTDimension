#ifndef _SETJMP_H
#define _SETJMP_H

#define JB_R4	0
#define JB_R5	1
#define JB_R6	2
#define JB_R7	3
#define JB_R8	4
#define JB_R9	5
#define JB_R10	6
#define JB_R11	7
#define JB_R12	8
#define JB_R13	9
#define JB_R14	10
#define JB_R15	11
#define JB_FP	12
#define JB_SP	13
#define JB_R1	14
#define JB_IPL	15	/* Interrupt Mask for splx(), in kernel mode */
#define JB_F2	16
#define JB_F3	17
#define JB_F4	18
#define JB_F5	19
#define JB_F6	20
#define JB_F7	21

#define JB_NREGS	(JB_F7 + 1)

#if !defined(LOCORE) && !defined(ASSEMBLER)
typedef int jmp_buf[JB_NREGS];

#ifndef __STRICT_BSD__
extern int setjmp(jmp_buf env);
extern void longjmp(jmp_buf env, int val);
#endif /* __STRICT_BSD__ */
#endif

#endif /* _SETJMP_H */
