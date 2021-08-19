#if ! defined(__NDREG_H__)
#define __NDREG_H__

#define SIZE_Y	832
#define SIZE_X	1152
#define NDROWBYTES SIZE_X*4
#define CURSORWIDTH 16
#define CURSORHEIGHT 16

#if defined(PROTOTYPE)
#define NDCSR_RESET860		(0x8 << 24)
#define NDCSR_INT860		(0x10 << 24)
#define NDCSR_KEN860		(0x20 << 24)
#define NDCSR_INTCPU		(0x40 << 24)
#define NDCSR_PANIC		(0x80 << 24)	/* Old name */
#define NDCSR_VBLANK		(0x80 << 24)	/* New name (new Altera part) */
#else /* !PROTOTYPE */
#define NDCSR_RESET860		0x00000001	/* Memory Controller CSR bits */
#define NDCSR_CS8		0x00000002
#define NDCSR_INT860ENABLE	0x00000004
#define NDCSR_INT860		0x00000008
#define NDCSR_BE_IEN		0x00000010
#define NDCSR_BE_INT		0x00000020
#define NDCSR_VBL_IEN		0x00000040
#define NDCSR_VBL_INT		0x00000080
#define NDCSR_VBLANK		0x00000100	/* Read only bit */
#define NDCSR_VIOVBL_IEN	0x00000200
#define NDCSR_VIOVBL_INT	0x00000400
#define NDCSR_VIOBLANK		0x00000800	/* Read only bit */
#define NDCSR_KEN860		0x00001000

#define NDCSR1_INTCPU		1		/* Interrupt host processor */
#define NDCSR2_GLOBAL_AEN	1		/* Global access enable */

#define NDDMA_CSR_VISIBLE_EN	0x01
#define NDDMA_CSR_BLANKED_EN	0x02
#define NDDMA_CSR_READ_EN	0x04

#define NDVID_TIMING_FORCE_VBLANK	0x01
#define NDVID_TIMING_60HZ_MODE		0x02
#define NDVID_TIMING_EXTERN_SYNC	0x04

#define ND_DRAM_SIZE_4_MBIT		1
#endif /* !PROTOTYPE */

#ifdef PROTOTYPE
#define P_ADDR_MEMORY		0xFF800000
#else
#define P_ADDR_MEMORY		0x08000000	/* Base of memory, board relative */
#endif

typedef volatile int v_int;			/* For use in macros... */

#if ! defined(ASSEMBLER)
#if defined(i860)

extern unsigned long		_slot_id_;
#define SLOT_ID			_slot_id_

#if defined(PROTOTYPE)
extern volatile int *__ND_csr;
#define ADDR_FRAMESTORE		((v_int *)0xFF400000)
#define ADDR_DRAM		((v_int *)0xFF800000)
#define ADDR_DAC_ADDR_PORT	((v_int *)(((char *)__ND_csr)-0x10))
#define ADDR_DAC_REG_PORT	((v_int *)(((char *)__ND_csr)-0xC))
#define ADDR_DAC_PALETTE_PORT	((v_int *)(((char *)__ND_csr)-0x8))
#define ADDR_NDP_CSR	 	(__ND_csr)

#define ND_RESET860		(*ADDR_NDP_CSR |= NDCSR_RESET860)
#define ND_INT860		(*ADDR_NDP_CSR |= NDCSR_INT860)
#define ND_KEN860		(*ADDR_NDP_CSR |= NDCSR_KEN860)
#define ND_INTCPU		(*ADDR_NDP_CSR |= NDCSR_INTCPU)
#define ND_IS_INTCPU		(*ADDR_NDP_CSR & NDCSR_INTCPU)
#define ND_IS_INT860		(*ADDR_NDP_CSR & NDCSR_INT860)
#define ND_IS_KEN860		(*ADDR_NDP_CSR & NDCSR_KEN860)
#define ND_IS_PANIC		(*ADDR_NDP_CSR & NDCSR_PANIC)
#define ND_IS_VBLANK		(*ADDR_NDP_CSR & NDCSR_VBLANK)

#else /* !PROTOTYPE */
/* Physical addresses and sizes used in pmap_bootstrap() */
#define PADDR_DRAM		abs_addr(ADDR_DRAM)	/* Untagged DRAM */
#define PADDR_DRAM_SIZE		0x04000000		/* 64 Mbytes */
#define PADDR_FRAMESTORE	abs_addr(ADDR_FRAMESTORE)/* 24 bit VRAM */
#define PADDR_FRAMESTORE_SIZE	0x00400000		/* 4 Mbytes */
#define PADDR_MC		0xFF800000		/* On-board MC regs */
#define PADDR_MC_SIZE		0x00004000
#define PADDR_DAC_PORT		0xFF200000		/* Bt463 part base addr */
#define PADDR_DAC_PORT_SIZE	0x00001000

/* The following addresses apply AFTER pmap_bootstrap() has run! */
#define ADDR_FRAMESTORE		((v_int *)(0x0E000000 | SLOT_ID)) /* 24 bit VRAM */
#define ADDR_DRAM		((v_int *)0xF8000000)	/* Untagged DRAM */
#define ADDR_TRAP_VECTOR	0xFFFFFF00	

#define ADDR_DAC_PORT ((volatile unsigned char*)0xFF200000) /* Base of DAC  */
#define ADDR_ND_CSR	 	((v_int *)0xFF800000)	/* By CSR, we mean CSR0... */
#define ADDR_ND_CSR0	 	((v_int *)0xFF800000)
#define ADDR_ND_CSR1	 	((v_int *)0xFF800010)	/* Host interrupt */
#define ADDR_ND_CSR2	 	((v_int *)0xFF800020)	/* Global Access Enable */
#define ADDR_ND_SLOTID	 	((v_int *)0xFF800030)	/* NextBus Slot ID */

#define ADDR_ND_DMA_CSR	 	((v_int *)0xFF801000)	/* DMA engine CSR */
#define ADDR_ND_DMA_VIS_START	((v_int *)0xFF801010)	/* DMA engine visible start */
#define ADDR_ND_DMA_VIS_WIDTH	((v_int *)0xFF801020)	/* DMA visible width: 1152  */
#define ADDR_ND_DMA_VMASK_START	((v_int *)0xFF801030)	/* start of pixel write mask */
#define ADDR_ND_DMA_VMASK_WIDTH	((v_int *)0xFF801040)	/* rowPixels of write mask */
#define ADDR_ND_DMA_BLANK_START	((v_int *)0xFF801050)	/* DMA engine blank start */
#define ADDR_ND_DMA_BLANK_WIDTH	((v_int *)0xFF801060)	/* DMA blank width: 1152  */
#define ADDR_ND_DMA_BMASK_START	((v_int *)0xFF801070)	/* start of pixel write mask */
#define ADDR_ND_DMA_BMASK_WIDTH	((v_int *)0xFF801080)	/* rowPixels of write mask */
#define ADDR_ND_DMA_VIS_TOPLINE	((v_int *)0xFF801090)	/* 1st visible scanline */
#define ADDR_ND_DMA_VIS_BOTLINE	((v_int *)0xFF8010A0)	/* last vis scanline (excl.) */
#define ADDR_ND_DMA_LINEADDR	((v_int *)0xFF8010B0)	/* line addr acc. (diag.) */
#define ADDR_ND_DMA_CURADDR	((v_int *)0xFF8010C0)	/* current addr (diag.) */
#define ADDR_ND_DMA_MLINEADDR	((v_int *)0xFF8010D0)	/* mask line addr (diag.) */
#define ADDR_ND_DMA_MCURADDR	((v_int *)0xFF8010E0)	/* mask current addr (diag.) */
#define ADDR_ND_DMA_OUTADDR	((v_int *)0xFF8010F0)	/* DMA output addr (diag.) */

#define ADDR_ND_VIDEO_TIMING	((v_int *)0xFF802000)	/* Video timing register */

#define ADDR_ND_DRAM_SIZE	((v_int *)0xFF803000)	/* DRAM (1 or 4 Mbit) reg */

/* various and sundry DataPath registers */
#define PHYSADDR_DATAPATH	((v_int *)((SLOT_ID>>4) | 0xF0000000))
#define OFFSET_DP_CSR		0x340
#define OFFSET_DP_VIDEO_ALPHA	0x344
#define OFFSET_DP_DMA_XFER_CNT	0x348
#define OFFSET_DP_XPHASE	0x350
#define OFFSET_DP_YPHASE	0x354
#define OFFSET_DP_IIC_1		0x360
#define OFFSET_DP_IIC_2		0x364

#define DP_CSR_MASK_DISABLE	(1 << 0)
#define DP_CSR_NTSC_ENABLE	(1 << 1)	/* off for NTSC */
#define DP_CSR_JPEG_IN_0	(1 << 2)
#define DP_CSR_JPEG_OUT_0	(1 << 3)
#define DP_CSR_JPEG_OE_0	(1 << 4)
#define DP_CSR_JPEG_IN_1	(1 << 5)
#define DP_CSR_JPEG_OUT_1	(1 << 6)
#define DP_CSR_JPEG_OE_1	(1 << 7)

#define ND_RESET860		(*ADDR_ND_CSR0 |= NDCSR_RESET860)
#define ND_INT860		(*ADDR_ND_CSR0 |= (NDCSR_INT860+NDCSR_INT860ENABLE))
#define ND_KEN860		(*ADDR_ND_CSR0 |= NDCSR_KEN860)
#define ND_INTCPU		(*ADDR_ND_CSR1 |= NDCSR1_INTCPU)
#define ND_IS_INTCPU		(*ADDR_ND_CSR1 & NDCSR1_INTCPU)
#define ND_IS_INT860		(*ADDR_ND_CSR0 & NDCSR_INT860)
#define ND_IS_KEN860		(*ADDR_ND_CSR0 & NDCSR_KEN860)
#define ND_IS_PANIC		(0)
#define ND_IS_VBLANK		(*ADDR_ND_CSR0 & NDCSR_VBLANK)

#endif /* !PROTOTYPE */

#else	/* ! i860 */
#if defined(KERNEL)	/* The kernel has each region mapped individually. */
/*
 * In the kernel, the board_addr field may only be used safely in the service threads.
 */
#if defined(PROTOTYPE)

#define ADDR_FRAMESTORE(ND_var)	\
	((volatile int *)(ND_var)->fb_addr)
#define ADDR_DRAM(ND_var) \
	((volatile int *)(((char *)(ND_var)->board_addr)+ (8 << 20)))
#define ADDR_DAC_ADDR_PORT(ND_var)	((volatile int *)(ND_var)->csr_addr)
#define ADDR_DAC_REG_PORT(ND_var) \
	((volatile int *)(((char *)(ND_var)->csr_addr)+4))
#define ADDR_DAC_PALETTE_PORT(ND_var) \
 	((volatile int *)(((char *)(ND_var)->csr_addr)+8))
#define ADDR_NDP_CSR(ND_var) \
 	((volatile int *)(((char *)(ND_var)->csr_addr)+0x10))
	
#else /* ! PROTOTYPE */

#define ADDR_FRAMESTORE(ND_var)		((volatile int *)(ND_var)->fb_addr)
#define ADDR_DRAM(ND_var) \
	((volatile int *)(((char *)(ND_var)->board_addr)+ (0x08000000)
#define ADDR_DAC_ADDR_PORT(ND_var)	((volatile char *)(ND_var)->bt_addr)
#define ADDR_DAC_REG_PORT(ND_var)	((volatile char *)(ND_var)->bt_addr)
#define ADDR_DAC_PALETTE_PORT(ND_var)	((volatile char *)(ND_var)->bt_addr)
#define ADDR_NDP_CSR(ND_var)	((volatile int *)((ND_var)->csr_addr))	/* Pun CSR0 */
#define ADDR_NDP_CSR0(ND_var)	((volatile int *)((ND_var)->csr_addr))
#define ADDR_NDP_CSR1(ND_var)	((volatile int *)(((char *)(ND_var)->csr_addr)+0x10))
	
#endif /* ! PROTOTYPE */

#else /* ! KERNEL */

#if defined(PROTOTYPE)

#define ADDR_FRAMESTORE(ND_var)	\
	((volatile int *)(((char *)(ND_var)->fb_addr)))
#define ADDR_DRAM(ND_var) \
	((volatile int *)(((char *)(ND_var)->dram_addr)))
#define ADDR_DAC_ADDR_PORT(ND_var)	((volatile int *)(ND_var->csr_addr))
#define ADDR_DAC_REG_PORT(ND_var) \
	((volatile int *)(((char *)(ND_var)->csr_addr)+4))
#define ADDR_DAC_PALETTE_PORT(ND_var) \
 	((volatile int *)(((char *)(ND_var)->csr_addr)+8))
#define ADDR_NDP_CSR(ND_var) \
 	((volatile int *)(((char *)(ND_var)->csr_addr)+0x10))

#else /* !PROTOTYPE */

#define ADDR_FRAMESTORE(ND_var)		((volatile int *)((ND_var)->fb_addr))
#define ADDR_DRAM(ND_var)		((volatile int *)((ND_var)->dram_addr))
#define ADDR_DAC_ADDR_PORT(ND_var)	((volatile char *)(ND_var->bt_addr))
#define ADDR_DAC_REG_PORT(ND_var)	((volatile char *)(ND_var->bt_addr))
#define ADDR_DAC_PALETTE_PORT(ND_var)	((volatile char *)(ND_var->bt_addr))
#define ADDR_NDP_CSR(ND_var)	((volatile int *)((ND_var)->csr_addr))	/* Pun CSR0 */
#define ADDR_NDP_CSR0(ND_var)	((volatile int *)((ND_var)->csr_addr))
#define ADDR_NDP_CSR1(ND_var)	((volatile int *)(((char *)(ND_var)->csr_addr)+0x10))

#endif /* !PROTOTYPE */

#endif /* KERNEL */

#if defined(PROTOTYPE)

#define ND_RESET860(ND_var)		(*ADDR_NDP_CSR(ND_var) |= NDCSR_RESET860)
#define ND_INT860(ND_var)		(*ADDR_NDP_CSR(ND_var) |= NDCSR_INT860)
#define ND_KEN860(ND_var)		(*ADDR_NDP_CSR(ND_var) |= NDCSR_KEN860)
#define ND_INTCPU(ND_var)		(*ADDR_NDP_CSR(ND_var) |= NDCSR_INTCPU)
#define ND_IS_INTCPU(ND_var)		(*ADDR_NDP_CSR(ND_var) & NDCSR_INTCPU)
#define ND_IS_PANIC(ND_var)		(*ADDR_NDP_CSR(ND_var) & NDCSR_PANIC)
#define ND_IS_VBLANK(ND_var)		(*ADDR_NDP_CSR(ND_var) & NDCSR_VBLANK)

#else /* !PROTOTYPE */

#define ND_RESET860(ND_var)		(*ADDR_NDP_CSR0(ND_var) |= NDCSR_RESET860)
#define ND_INT860(ND_var) (*ADDR_NDP_CSR0(ND_var) |= (NDCSR_INT860+NDCSR_INT860ENABLE))
#define ND_KEN860(ND_var)		(*ADDR_NDP_CSR0(ND_var) |= NDCSR_KEN860)
#define ND_INTCPU(ND_var)		(*ADDR_NDP_CSR1(ND_var) |= NDCSR1_INTCPU)
#define ND_IS_INTCPU(ND_var)		(*ADDR_NDP_CSR1(ND_var) & NDCSR1_INTCPU)
#define ND_IS_INT860(ND_var)		(*ADDR_NDP_CSR0(ND_var) & NDCSR_INT860)
#define ND_IS_KEN860(ND_var)		(*ADDR_NDP_CSR0(ND_var) & NDCSR_KEN860)
#define ND_IS_PANIC(ND_var)		(0)
#define ND_IS_VBLANK(ND_var)		(*ADDR_NDP_CSR0(ND_var) & NDCSR_VBLANK)


#endif /* !PROTOTYPE */

#endif
#if defined(i860)
/*
 * Physical addresses of hardware on the ND board
 */
#if defined(PROTOTYPE)
#define ND_BASE_DRAM	0xFF800000
#define ND_END_DRAM	0xFFFFF000	/* Last page is special */

#define ND_BASE_VRAM	0xFF400000
#define ND_END_VRAM	0xFF800000

#define ND_CSR_REGS		0x00000000	/* Phys addr of CSR */
#define ND_CSR_VIRT_REGS	0x01000000	/* Virt addr of CSR */

#define ND_PAGE_SIZE	0x1000

typedef struct ND_csr_struct
{
	unsigned long DAC_addr;
	unsigned long DAC_data;
	unsigned long DAC_clut;
	unsigned long _dummy1_;
	unsigned long csr1;
	unsigned long _dummy2_;
	unsigned long csr2;
} ND_csr_t;

#else /* !PROTOTYPE */
#define ND_BASE_DRAM	PADDR_DRAM
#define ND_END_DRAM	(PADDR_DRAM + PADDR_DRAM_SIZE)

#define ND_BASE_VRAM	PADDR_FRAMESTORE
#define ND_END_VRAM	(PADDR_FRAMESTORE + PADDR_FRAMESTORE_SIZE)

#define ND_CSR_REGS		PADDR_MC	/* Phys addr of CSR */
#define ND_CSR_VIRT_REGS	ND_CSR_REGS	/* Virt addr of CSR */

#define ND_PAGE_SIZE	0x1000

#endif /* !PROTOTYPE */

#endif /* i860 */

#endif /* ! ASSEMBLER */
#endif /* __NDREG_H__ */


