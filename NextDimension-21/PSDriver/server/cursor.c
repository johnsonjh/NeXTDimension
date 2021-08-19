/*****************************************************************************

    cursor.c
    remote cursor manipulation routines
    
    CONFIDENTIAL
    Copyright 1990 NeXT Computer, Inc. as an unpublished work.
    All Rights Reserved.

    01Aug90 Ted Cohn
    
    Modifications:

******************************************************************************/

#import "package_specs.h"

#ifdef I860
void set_cursor (LocalBitmap *lbm)
{
    volatile unsigned int *cp, *data;
    short i, j, rowLongs;
    
    data = lbm->bits;
    rowLongs = lbm->rowBytes >> 2;
    cp = cursorData32;
    
    switch(lbm->base.type) {
    case NX_TWOBITGRAY: {
	unsigned int d, m, e, f;
	volatile unsigned int *mask;
	
	mask = lbm->abits;
	for (i=16; --i>=0; ) {
	    m = *mask; mask += rowLongs;	/* Get mask data and inc ptr */
	    d = *data; data += rowLongs;	/* Get grey data and convert */
	    for (j=30; j>=0; j-=2) {
		e = (d>>j) & 3;		/* isolate single 2bit data */
		e = (e << 2) | e;
		e = (e << 4) | e;
		e = (e << 8) | e;
		e = ((e << 16) | e) & 0xFFFFFF00;
		f = (m>>j) & 3;		/* isolate single 2bit mask (alpha) */
		f = (f << 2) | f;
		e |= (f << 4) | f;
		*cp++ = e;
	    }
	}
	break;
    }
    case NX_TWENTYFOURBITRGB:
	for (i=16; --i>=0; ) {
		*cp++ = *data++;
		*cp++ = *data++;
		*cp++ = *data++;
		*cp++ = *data++;
		*cp++ = *data++;
		*cp++ = *data++;
		*cp++ = *data++;
		*cp++ = *data++;
		*cp++ = *data++;
		*cp++ = *data++;
		*cp++ = *data++;
		*cp++ = *data++;
		*cp++ = *data++;
		*cp++ = *data++;
		*cp++ = *data++;
		*cp++ = *data++;
	}
	break;
    }
}

/* need framebuffer bits, rowbytes, screen bounds */
void remove_cursor(Bounds saveRect)
{
    short i, j, width, vramRow;
    volatile unsigned int *vramPtr;
    volatile unsigned int *savePtr;
    
    savePtr = saveData;
    vramPtr = ((LocalBitmap *)device->bm)->bits;
    vramRow = ((LocalBitmap *)device->bm)->rowBytes >> 2;
    vramPtr += (vramRow * (saveRect.miny - screenRect.miny)) +
	       (saveRect.minx - screenRect.minx);
    width = saveRect.maxx - saveRect.minx;
    vramRow -= width;
    for (i = saveRect.maxy - saveRect.miny; --i>=0; ) {
	for (j = width; --j>=0; ) *vramPtr++ = *savePtr++;
	vramPtr += vramRow;
    }
}

/* need screen bounds, bits, rowbytes and screen save buffer */
void display_cursor(Bounds saveRect, Bounds cursorRect)
{
    int i, j, width, cursRow, vramRow;
    Bounds bounds, saveRect;
    volatile unsigned int *vramPtr;
    volatile unsigned int *savePtr;
    volatile unsigned int *cursPtr;
    unsigned int s, d, f, rbmask=0xFF00FF00, gamask=0xFF00FF;

    bounds = device->bounds;
    
    vramPtr = ((LocalBitmap *)device->bm)->bits;
    vramRow = ((LocalBitmap *)device->bm)->rowBytes >> 2;
    vramPtr += (vramRow * (saveRect.miny - bounds.miny)) +
	       (saveRect.minx - bounds.minx);
    width = saveRect.maxx - saveRect.minx;
    vramRow -= width;
    cursPtr = cursorData32;
    cursPtr += (saveRect.miny - cursorRect.miny) * CURSORWIDTH +
		saveRect.minx - cursorRect.minx;
    savePtr = saveData;
    cursRow = CURSORWIDTH - width;
    
    for (i = saveRect.maxy-saveRect.miny; i>0; i--) {
	for (j = width; j>0; j--) {
	    d = *savePtr++ = *vramPtr;
	    s = *cursPtr++;
	    f = (~s) & (unsigned int)0xFF;
	    d = s + (((((d & rbmask)>>8)*f + gamask) & rbmask)
		| ((((d & gamask)*f+gamask)>>8) & gamask));
	    *vramPtr++ = d;
	}
	cursPtr += cursRow; /* starting point of next cursor line */
	vramPtr += vramRow; /* starting point of next screen line */
    }
}
#endif



