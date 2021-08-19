
#include <mach.h>
#include "video.h"

vm_offset_t dp_base_addr;
static vm_offset_t vmask_addr, pmask_addr;

static inline SetDataPathReg(int offset,int val)
{
    vm_offset_t addr = dp_base_addr + offset;
    (*(v_int *)(dp_base_addr + offset)) = val;
}

void video_ntsc_setup()
{
    /* poke the hardware into a sane state (eventually, we should initialize
       the iic stuff at this point */

    SetDataPathReg(OFFSET_DP_CSR,
/*		   DP_CSR_MASK_DISABLE | */
		   DP_CSR_NTSC_ENABLE |
		   DP_CSR_JPEG_OE_0 |
		   DP_CSR_JPEG_OE_1);
    SetDataPathReg(OFFSET_DP_VIDEO_ALPHA, 0xff);

    /* configure the dma engine */
    
    /* these are the NTSC specific settings */
    *ADDR_ND_DMA_VIS_TOPLINE = NTSC_FIRST_LINE;
    *ADDR_ND_DMA_VIS_BOTLINE = NTSC_LAST_LINE;
    SetDataPathReg(OFFSET_DP_DMA_XFER_CNT, NTSC_PACKETS);
}
void video_init()
{
    kern_return_t r;
    /* first, map in the data path chip registers */
#define FOO (0xff000000) /* | (SLOT_ID >> 4)) */
    if(r = vm_map_hardware(task_self(),&dp_base_addr, FOO,
			   ND_PAGE_SIZE, TRUE))
	os_raise(r, "error mapping in datapath registers");

    /* allocate some wired memory for the mask bitmap */

    if(r = vm_allocate_wired(task_self(), &vmask_addr, &pmask_addr,
			     VIDEO_MASKSIZE,
		       TRUE))
	os_raise(r, "error allocating wired memory for video mask");
    vm_cacheable( task_self(), vmask_addr, VIDEO_MASKSIZE, FALSE);

    bzero(vmask_addr,VIDEO_MASKSIZE);

    *ADDR_ND_DMA_VMASK_START = pmask_addr;
    *ADDR_ND_DMA_VMASK_WIDTH = VIDEO_MASK_ROWBYTES;
    *ADDR_ND_DMA_CSR = 0;
    video_ntsc_setup();
    
}

static inline video_wait()
{
    int i = 500000;  /* at least 1/60th of a second */
    
    while ((*ADDR_ND_CSR & NDCSR_VIOBLANK) && --i > 0)
	;
    if( i == 0)
	printf("video_wait(): Video Retrace Bit stuck on!\n");
    i = 500000;
    while (!(*ADDR_ND_CSR & NDCSR_VIOBLANK) && --i > 0)
	;
    if( i == 0)
	printf("video_wait(): Video Retrace Bit stuck off!\n");
    
}

void video_dma_addr(void *p, int rowbytes)
{

    
    /* convert pointer to physical address */
    int paddr = kvtophys(p);
    video_wait();
    *ADDR_ND_DMA_VIS_START = paddr;
    *ADDR_ND_DMA_VIS_WIDTH = rowbytes;
}


void video_dma_start()
{
    video_wait();
    /* let'er rip */
    *ADDR_ND_DMA_CSR = NDDMA_CSR_VISIBLE_EN;
}
void video_dma_stop()
{
    video_wait();
    *ADDR_ND_DMA_CSR = 0;
}

extern unsigned int leftBitArray[], rightBitArray[];

void video_mask_clearrect(int x1, int y1, int x2, int y2)
{

    int first_word, last_word;
    unsigned int lmask, rmask;
    int k, midcnt, h;
    unsigned char *rowptr;

    /* clip and sort input coordinates */
    if(x1 > x2) x1 ^= x2 ^= x1 ^= x2;
    if(y1 > y2) y1 ^= y2 ^= y1 ^= y2;
    if(x1 < 0) x1 = 0;
    if(x2 > VIDEO_WIDTH) x2 = VIDEO_WIDTH;
    if(y1 < 0) y1 = 0;
    if(y2 > VIDEO_HEIGHT) y2 = VIDEO_HEIGHT;
    
    first_word = x1 >> 5;
    last_word = x2 >> 5;
    lmask = ~leftBitArray[x1 & 31];
    rmask = ~rightBitArray[x2 & 31];
    h = y2 - y1;
    
    rowptr = (unsigned char *)(vmask_addr + y1*VIDEO_MASK_ROWBYTES +
			       (first_word << 2));
    midcnt = last_word - first_word - 1;
    
    if(midcnt == -1) {  /* i.e. first_word == last_word */
	rmask = rmask | lmask;
	for(; --h >= 0;rowptr += VIDEO_MASK_ROWBYTES) {
	    unsigned int *p = (unsigned int *)rowptr;
	    *p &= rmask;
	}
    } else {
	for(; --h >= 0; rowptr += VIDEO_MASK_ROWBYTES) {
	    unsigned int *p = (unsigned int *)rowptr;

	    *p++ &= lmask;
	    for(k = midcnt; --k >= 0;)
		*p++ = 0;
	    *p++ &= rmask;
	}
    }
}


	
void video_mask_setrect(int x1, int y1, int x2, int y2)
{

    int first_word, last_word;
    unsigned int lmask, rmask;
    int k, midcnt, h;
    unsigned char *rowptr;

    /* clip and sort input coordinates */
    if(x1 > x2) x1 ^= x2 ^= x1 ^= x2;
    if(y1 > y2) y1 ^= y2 ^= y1 ^= y2;
    if(x1 < 0) x1 = 0;
    if(x2 > VIDEO_WIDTH) x2 = VIDEO_WIDTH;
    if(y1 < 0) y1 = 0;
    if(y2 > VIDEO_HEIGHT) y2 = VIDEO_HEIGHT;
    
    first_word = x1 >> 5;
    last_word = x2 >> 5;
    lmask = leftBitArray[x1 & 31];
    rmask = rightBitArray[x2 & 31];
    h = y2 - y1;

    rowptr = (unsigned char *)(vmask_addr + y1*VIDEO_MASK_ROWBYTES +
			       (first_word << 2));
    midcnt = last_word - first_word - 1;
    if(midcnt == -1) {  /* i.e. first_word == last_word */
	rmask = rmask & lmask;
	for(; --h >= 0;rowptr += VIDEO_MASK_ROWBYTES) {
	    unsigned int *p = (unsigned int *)rowptr;
	    *p |= rmask;
	}
    } else {
	for(; --h >= 0; rowptr += VIDEO_MASK_ROWBYTES) {
	    unsigned int *p = (unsigned int *)rowptr;
	    
	    *p++ |= lmask;
	    for(k = midcnt; --k >= 0;)
		*p++ = 0xffffffff;
	    *p++ |= rmask;
	}
    }
}


