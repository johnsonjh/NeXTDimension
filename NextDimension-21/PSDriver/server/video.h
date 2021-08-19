
#include <ND/NDlib.h>

#define VIDEO_DMA_PACKET_SIZE  64


#define NTSC_WIDTH  	640
#define NTSC_HEIGHT	480
#define NTSC_PACKETS (NTSC_WIDTH/VIDEO_DMA_PACKET_SIZE)
#define NTSC_FIRST_LINE 38
#define NTSC_LAST_LINE  (NTSC_HEIGHT + NTSC_FIRST_LINE)



/* eventually, we want to make this large enough to support NTSC
   or PAL
   */
#define VIDEO_WIDTH  NTSC_WIDTH	/* must be multiple of 64 */
#define VIDEO_HEIGHT NTSC_HEIGHT
#define VIDEO_MASK_ROWBYTES (VIDEO_WIDTH/8)
#define VIDEO_MASKSIZE  (VIDEO_MASK_ROWBYTES * VIDEO_HEIGHT)


