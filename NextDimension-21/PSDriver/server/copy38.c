/*****************************************************************************
    copy38.c 
    
    Copyright (c) 1988 NeXT Incorporated.
    All rights reserved.

*****************************************************************************/


#define uint unsigned int
#define uchar unsigned char

/* srcbase is byte-aligned, dstbase is long-aligned
 */

Copy38(int width, int height, uchar *srcbase, uint *dstbase,
       int sbw, int dbw, int alpha)
{
    uchar *sp;
    uint *dp;
    int w;
    
    if (alpha)
	do {
	    sp = srcbase;
	    dp = dstbase;
	    w = width;
	    do {
		*dp++ = ((*sp)<<24) | (*(sp+1)<<16) |
			(*(sp+2)<<8) | (*(sp+3));
		sp += 4;
	    } while (--w != 0);
	    srcbase += sbw;
	    dstbase = (uint *)((uchar *)dstbase + dbw);
	} while (--height != 0);
    else
        do {
	    sp = srcbase;
	    dp = dstbase;
	    w = width;
	    do {
		*dp++ = ((*sp)<<24) | (*(sp+1)<<16) |
			(*(sp+2)<<8) | 0xFF;
		sp += 3;
	    } while (--w != 0);
	    srcbase += sbw;
	    dstbase = (uint *)((uchar *)dstbase + dbw);
	} while (--height != 0);
}
