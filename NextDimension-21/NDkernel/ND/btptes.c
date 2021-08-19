/*
 * Pages used at startup to configure the kernel VM system.
 */
/* Assign a non-zero value, forcing these to reside in the data seg, all aligned. */
#if 0
long	kpde[1024] = {1};
long	iopte[1024] = {1};
long	vram_pte[1024] = {1};
long	dram_lo_pte[1024] = {1};
long	dram_hi_pte[1024] = {1};
#endif
char bad[2*4096] = {1};


