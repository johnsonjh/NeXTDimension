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

#define PSR_PM		0xFF000000	/* Bits used to construct the pixel mask */
#define PSR_PMSHIFT	24
#define PSR_PSMASK	0x00c00000	/* Bit field describing pixel size */
#define PSR_PS8		0		/* Contents of PSR_PSMASK for 8 bit pixels */
#define PSR_PS16	0x00400000	/* Contents of PSR_PSMASK for 16 bit pixels */
#define PSR_PS32	0x00800000	/* Contents of PSR_PSMASK for 32 bit pixels */

/*
 *	i860 extended processor status register
 */
#define	EPSR_PTMASK	0x000000FF	/* Processor type field */
#define EPSR_PTi860	1		/* Processor type for i860 */

#define EPSR_STEPMASK	0x00001F00	/* Processor step encoding */
#define EPSR_IL		0x00002000	/* Interlock flag for trap handler */
#define EPSR_WP		0x00004000	/* Set semantics for PTE_WRITABLE bit */
#define EPSR_INT	0x00020000	/* Value of INT input pin */
#define EPSR_DCS	0x003C0000	/* Data cache size info */
#define EPSR_PBM	0x00400000	/* PTE_NOCACHE or PTE_WRITETHRU on PTB pin */
#define EPSR_BE		0x00800000	/* Select big-endian data ordering */
#define EPSR_OF		0x01000000	/* Integer overflow flag */

/*
 *	i860 direct-ory pointer
 */

#define DIR_ATE		0x00000001	/* address translation enable */
#define DIR_ITI		0x00000020	/* tlb & i-cache invalidate  */
#define DIR_CS8		0x00000080	/* CS8 mode enabled  */
