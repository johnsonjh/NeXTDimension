/*
 *         INTEL CORPORATION PROPRIETARY INFORMATION
 *
 *    This software is supplied under the terms of a license 
 *    agreement or nondisclosure agreement with Intel Corpo-
 *    ration and may not be copied or disclosed except in
 *    accordance with the terms of that agreement.
 */

/*
 * i860 processor status register
 */

#define PSR_BR		0x00000001	/* break read */
#define PSR_BW		0x00000002	/* break write */
#define PSR_CC		0x00000004	/* condition code */
#define PSR_LCC		0x00000008	/* loop condition code */

#define PSR_IM		0x00000010	/* interrupt mode */
#define PSR_PIM		0x00000020	/* prev. interrupt mode */
#define PSR_U		0x00000040	/* user mode */
#define PSR_PU		0x00000080	/* prev. user mode */

#define PSR_IT		0x00000100	/* instruction trap */
#define PSR_IN		0x00000200	/* interrupt */
#define PSR_IAT		0x00000400	/* instruction access trap */
#define PSR_DAT		0x00000800	/* data access trap */
#define PSR_FT		0x00001000	/* floating-point trap */

#define PSR_DS		0x00002000	/* delayed switch */
#define PSR_DIM		0x00004000	/* dual instruction mode */
#define PSR_KNF		0x00008000	/* kill next float instr. */

#define PSR_USERSET	(PSR_U)		/* bits to set for ptrace */
#define PSR_USERCLR	(PSR_IM+PSR_PIM) /* bits to clear for ptrace */

/*
 *	i860 direct-ory pointer
 */

#define DIR_ATE		0x00000001	/* address translation enable */
#define DIR_ITI		0x00000020	/* tlb & i-cache invalidate  */
