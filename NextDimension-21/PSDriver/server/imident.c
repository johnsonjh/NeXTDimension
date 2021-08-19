/*****************************************************************************
    imident.c 
    
    Copyright (c) 1988 NeXT Incorporated.
    All rights reserved.

    THIS MODULE DEPENDS ON BYTE-ALIGNED LONGWORD ACCESS

    Edit History:
	26Sep88 Jack	created using outer loop structure of ims12d12notfr.c
	18Apr90 Terry	Optimized code size (2372 to 1888 bytes)
	
*****************************************************************************/

#import PACKAGE_SPECS
#import DEVICE
#import EXCEPT		/* for DebugAssert */
#import DEVPATTERN
#import DEVIMAGE

/* Variable declarations from framedev.h in device package */
extern integer framebytewidth;
extern PSCANTYPE framebase;
extern Card8 framelog2BD;

#define uint unsigned int
#define uchar unsigned char

extern PImageProcs fmImageProcs;
void Copy38(int width, int height, uchar *srcbase, uint *dstbase,
       int sbw, int dbw, int alpha);

#ifdef i860
#define GetLong(s,sp) \
    (s=*sp, s<<=8, s|=*(sp+1), s<<=8, s|=*(sp+2), s<<=8, s|=*(sp+3), sp+=4)

/* assumes rmask != 0 !!!! */
static inline uint CarefulGetLong(uchar *sp, int rmask)
{
    uint s;
    if(rmask & 0x000000ff) {
	s=*sp, s<<=8, s|=*(sp+1), s<<=8, s|=*(sp+2), s<<=8, s|=*(sp+3);	
    } else if (rmask & 0x0000ffff) {
	s=*sp, s<<=8, s|=*(sp+1), s<<=8, s|=*(sp+2), s<<=8;
    } else if(rmask & 0x00ffffff) {
	s = *sp, s<<=8, s|= *(sp+1), s <<=16;
    } else
	s = (*sp << 24);
    return s;
}


void ByteCopy(uchar *sCur, uint *destPtr, int numInts, int lShift,
	      uint lMask, uint rMask)
{
    int rShift = SCANUNIT - lShift;
    int msk;
    uint us, s;

    if (lShift) { /* Unaligned case */
	s = 0;
	if (lMask >> lShift) {
	    GetLong(s, sCur);
	    s = ~s << lShift;
	}
	else sCur += 4;
	if(numInts == 0) {
	    if(lMask << rShift) {
		us = CarefulGetLong(sCur,lMask << rShift);
		sCur += 4;
	    }
	    else
		us = 0;
	}
	else
	    GetLong(us, sCur);
	us = ~us;
	s |= us >> rShift;

	*destPtr = (*destPtr & ~lMask) | (s & lMask); destPtr++;
	for (msk = numInts - 1; msk > 0; msk--) {
	    s = us << lShift;
	    GetLong(us, sCur);
	    us = ~us;
	    *destPtr++ = s | (us >> rShift);
	}
	if (numInts > 0) {
	    if (rMask << rShift) {
		s = CarefulGetLong(sCur,rMask);
		*destPtr = (*destPtr & ~rMask) |
		(((us << lShift) | (~s >> rShift)) & rMask);
	    }
	    else
		*destPtr = (*destPtr & ~rMask) | ((us << lShift) & rMask);
	}
    } else { /* Aligned case */
	if(numInts == 0) {
	    s = CarefulGetLong(sCur,lMask);
	    sCur += 4;
	}
	else
	    GetLong(s, sCur);
	*destPtr = (*destPtr & ~lMask) | (~s & lMask); destPtr++;
	for (msk = numInts - 1; msk > 0; msk--) {
	    GetLong(s, sCur);
	    *destPtr++ = ~s;
	}
	if (numInts > 0 && rMask) {
	    s = CarefulGetLong(sCur, rMask);
	    *destPtr = (*destPtr & ~rMask) | (~s & rMask);
	}
    }
}
#endif

/* Identity matrix, no transfer function, we invert source samples */
public procedure ImIdent(int items, DevTrap *t, DevRun *run, ImageArgs *args)
{
    DevMarkInfo *info = args->markInfo;
    DevImage *image = args->image;
    DevImageSource *source = image->source;
    unsigned int *destbase, lMask, rMask;
    unsigned char *srcByteBase;
    DevShort *buffptr;
    int y, ylast, pairs, xoffset, xl, xr, xSrcOffset, ySrcOffset;
    boolean leftSlope, rightSlope, spanChange;
    DevTrap trap;
    Mtx *mtx = image->info.mtx;
    real err;
    int unitl, numInts, lShift, rShift, ymin, ymax, xmin, xmax;
    
    DebugAssert((mtx->b == 0.0) && (mtx->c == 0.0)
		&& (args->procs == fmImageProcs)
		&& (err = mtx->a-1) < .0001 && err > -.0001
		&& (err = mtx->d-1) < .0001 && err > -.0001);
    DebugAssert( /* If one of these fails, it may increase my understanding */
	(image->info.sourcebounds.x.l == image->info.sourceorigin.x) &&
	(image->info.sourcebounds.y.l == image->info.sourceorigin.y) &&
	(image->info.sourcebounds.x.l == 0));

  xoffset = info->offset.x;
  ySrcOffset = (integer)mtx->ty - image->info.sourcebounds.y.l;
  xSrcOffset = (integer)mtx->tx - xoffset; /* both in pixels */
  xSrcOffset <<= framelog2BD; /* now bits */
  ymin = -ySrcOffset;
  ymax = ymin +
      (image->info.sourcebounds.y.g - image->info.sourcebounds.y.l) - 1;
  xmin = -xSrcOffset;
  xmax = xmin + (image->info.sourcebounds.x.g << framelog2BD);
  lShift = SCANMASK & xSrcOffset;
  rShift = SCANUNIT - lShift;
  while (--items >= 0) {
    if (run != NULL) {
      y = run->bounds.y.l;
      ylast = run->bounds.y.g - 1;
      buffptr = run->data;
    } else if (t != NULL) {
      trap = *t;
      y = trap.y.l;
      ylast = trap.y.g - 1;
      if (leftSlope = (trap.l.xl != trap.l.xg))
	trap.l.ix += Fix(xoffset);
      if (rightSlope = (trap.g.xl != trap.g.xg))
	trap.g.ix += Fix(xoffset);
      xl = trap.l.xl + xoffset;
      xr = trap.g.xl + xoffset;
    } else
      CantHappen();
    spanChange = 1;
    if (y<ymin)
      if (ylast<ymin) break; else y = ymin;
    if (ylast>ymax)
      if (y>ymax) break; else ylast = ymax;
    srcByteBase = source->samples[IMG_GRAY_SAMPLES]
      + (y + ySrcOffset) * source->wbytes + ((xSrcOffset >> SCANSHIFT) << 2);
    destbase = (uint *)((uchar *)framebase +(y+info->offset.y)*framebytewidth);
    while (true) { /* loop through all scanlines */
      pairs = (run != NULL) ? *(buffptr++) : 1;
      while (--pairs >= 0) {
        if (spanChange) {
	    if (run != NULL) {
		xl = xoffset + *buffptr++;
		xr = xoffset + *buffptr++;
	    }
	    else spanChange = leftSlope | rightSlope;

	    xl <<= framelog2BD;  xr <<= framelog2BD;
	    if (xl<xmin) {
		if (xr<xmin) {
		    /* rectangular trap completely clipped -> bail */
		    if(!spanChange) goto bail;
		    continue;
		}
		else xl = xmin;
	    }
	    if (xr>xmax) {
		if (xl>xmax) {
		    /* rectangular trap completely clipped -> bail */
		    if(!spanChange) goto bail;
		    continue;
		}
		else xr = xmax;
	    }
	    unitl = xl >> SCANSHIFT;
	    numInts = (xr >> SCANSHIFT) - unitl;
	    lMask = leftBitArray[xl & SCANMASK];
	    rMask = rightBitArray[xr & SCANMASK];
	    if (numInts == 0)
		lMask &= rMask;
	}
	{
	uint *sCur = (uint *)srcByteBase + unitl;
	uint *destPtr = destbase + unitl;
	int msk;
	  
#ifdef i860
	/* If source is not longword aligned, copy bytes on i860 */
	if ((int)srcByteBase & 0x3)
	    ByteCopy((uchar *)sCur,destPtr,numInts,lShift,lMask,rMask);
	else {
#endif

	if (lShift) { /* Unaligned case */
	  uint us, s;

	  s = 0;
	  if (lMask >> lShift) s = ~(*sCur) << lShift;
	  sCur++;
	  us  = ~(*sCur++);
	  s |= us >> rShift;

	  *destPtr = (*destPtr & ~lMask) | (s & lMask); destPtr++;
	  for (msk = numInts - 1; msk > 0; msk--) {
	    s = us << lShift;
	    us = ~(*sCur++);
	    *destPtr++ = s | (us >> rShift);
	  }
	  if (numInts > 0) {
	    if (rMask << rShift)
	      *destPtr = (*destPtr & ~rMask) |
		(((us << lShift) | (~(*sCur) >> rShift)) & rMask);
	    else
	      *destPtr = (*destPtr & ~rMask) | ((us << lShift) & rMask);
	  }
	} else { /* Aligned case */
	  *destPtr = (*destPtr & ~lMask) | (~(*sCur++) & lMask); destPtr++;
	  for (msk = numInts - 1; msk > 0; msk--)
	    *destPtr++ = ~(*sCur++);
	  if (numInts > 0 && rMask)
	      *destPtr = (*destPtr & ~rMask) | (~(*sCur) & rMask);
	}
#ifdef i860
	}
#endif
	}
      }
      if (++y > ylast)
	break;
      srcByteBase += source->wbytes;
      destbase = (uint *)((uchar *)destbase + framebytewidth);
      if (t != NULL && spanChange) {
	if (y == ylast) {
	  xl = trap.l.xg + xoffset;
	  xr = trap.g.xg + xoffset;
	} else {
	  if (leftSlope) {
	    xl = IntPart(trap.l.ix);
	    trap.l.ix += trap.l.dx;
	  }
	  if (rightSlope) {
	    xr = IntPart(trap.g.ix);
	    trap.g.ix += trap.g.dx;
	  }
	}
      }
    }
 bail:
    if (t != NULL)
      t++;
  }
}


public procedure ImIdent38(int items, DevTrap *t, DevRun *run, ImageArgs *args)
{
  DevMarkInfo *info = args->markInfo;
  DevImage *image = args->image;
  DevImageSource *source = image->source;
  DevShort *buffptr;
  int y, ylast, pairs, xoffset, yoffset, xl, xr, xSrcOffset, ySrcOffset;
  boolean leftSlope, rightSlope;
  DevTrap trap;
  Mtx *mtx = image->info.mtx;
  int ymin, ymax, xmin, xmax;
  int width, height, w, alpha;
  int sbw, dbw;
  uchar *srcByteBase, *sp;
  uint *destbase, *dp;

  alpha = args->image->unused;
  xoffset = info->offset.x;
  yoffset = info->offset.y;
  ySrcOffset = (integer)mtx->ty - image->info.sourcebounds.y.l;
  xSrcOffset = (integer)mtx->tx - xoffset; /* both in pixels */

  ymin = -ySrcOffset;
  ymax = ymin +
    (image->info.sourcebounds.y.g - image->info.sourcebounds.y.l) - 1;
  xmin = -xSrcOffset;
  xmax = xmin + image->info.sourcebounds.x.g;

  while (--items >= 0) {
    if (run != NULL) {
      y = run->bounds.y.l;
      ylast = run->bounds.y.g - 1;
      buffptr = run->data;
    } else if (t != NULL) {
      trap = *t;
      y = trap.y.l;
      ylast = trap.y.g - 1;
      leftSlope = (trap.l.xl != trap.l.xg);
      rightSlope = (trap.g.xl != trap.g.xg);
      xl = trap.l.xl + xoffset;
      xr = trap.g.xl + xoffset;
      if (xl<xmin)
	if (xr<xmin) xl = xr = xmin; else xl = xmin;
      if (xr>xmax)
	if (xl>xmax) xl = xr = xmax; else xr = xmax;
      if (leftSlope)
	trap.l.ix += Fix(xoffset);
      if (rightSlope)
	trap.g.ix += Fix(xoffset);
    } else
      CantHappen();
    if (y<ymin)
      if (ylast<ymin) break; else y = ymin;
    if (ylast>ymax)
      if (y>ymax) break; else ylast = ymax;

    sbw = source->wbytes;
    dbw = framebytewidth;
    srcByteBase = source->samples[IMG_INTERLEAVED_SAMPLES]
      + (y+ySrcOffset)*sbw
      + (alpha ? xSrcOffset<<2 : xSrcOffset + xSrcOffset + xSrcOffset);
    destbase = (uint *)((uchar *)framebase + (y+yoffset)*dbw);

    /* Special case rectangular trapezoid image */
    if(t && !leftSlope && !rightSlope) {
	width  = xr - xl;
	height = ylast + 1 - y;
	if(width > 0 && height > 0)
	    Copy38(width, height,
		    srcByteBase + (alpha ? xl<<2 : xl + xl + xl),
		    destbase + xl,
		    sbw, dbw, alpha);
    }
    else
	while (true) { /* loop through all scanlines */
	    pairs = (run != NULL) ? *buffptr++ : 1;
	    while (--pairs >= 0) {
		if (run != NULL) {
		    xl = xoffset + *buffptr++;
		    xr = xoffset + *buffptr++;
		}

		if (xl<xmin)
		    if (xr<xmin) continue; else xl = xmin;
		if (xr>xmax)
		    if (xl>xmax) continue; else xr = xmax;
		if(xl==xr) continue;

		w  = xr - xl;
		xl <<= 2; /* convert to bytes */
		sp = srcByteBase + xl;
		dp = (uint *) ((int)destbase + xl);
		if (alpha)
		    do {
			*dp++ = ((*sp)<<24) | (*(sp+1)<<16) |
				(*(sp+2)<<8) | (*(sp+3));
			sp += 4;
		    } while (--w != 0);
		else
		    do {
			*dp++ = ((*sp)<<24) | (*(sp+1)<<16) |
				(*(sp+2)<<8) | 0xFF;
			sp += 3;
		    } while (--w != 0);
	    }
	    if (++y > ylast) break;
	    srcByteBase += sbw;
	    destbase = (uint *)((uchar *)destbase + dbw);
	    if (t) {
		if (y == ylast) {
		    xl = trap.l.xg + xoffset;
		    xr = trap.g.xg + xoffset;
		} else {
		    if (leftSlope) {
			xl = IntPart(trap.l.ix);
			trap.l.ix += trap.l.dx;
		    }
		    if (rightSlope) {
			xr = IntPart(trap.g.ix);
			trap.g.ix += trap.g.dx;
		    }
		}
	    }
        }
     if (t != NULL) t++;
  }
}




