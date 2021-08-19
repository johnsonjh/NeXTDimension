/* PostScript generic image module

Copyright (c) 1983, '84, '85, '86, '87, '88 Adobe Systems Incorporated.
All rights reserved.

NOTICE:  All information contained herein is the property of Adobe Systems
Incorporated.  Many of the intellectual and technical concepts contained
herein are proprietary to Adobe, are protected as trade secrets, and are made
available only to Adobe licensees for their internal use.  Any reproduction
or dissemination of this software is strictly forbidden unless prior written
permission is obtained from Adobe.

PostScript is a registered trademark of Adobe Systems Incorporated.
Display PostScript is a trademark of Adobe Systems Incorporated.

Original version: Doug Brotz: Sept. 13, 1983
Edit History:
Doug Brotz: Mon Aug 25 14:41:39 1986
Chuck Geschke: Mon Aug  6 23:25:04 1984
Ed Taft: Wed Dec 13 18:17:32 1989
Ivor Durham: Wed Mar 23 13:11:32 1988
Bill Paxton: Tue Mar 29 09:25:41 1988
Paul Rovner: Wednesday, January 27, 1988 4:28:47 PM
Jim Sandman: Fri Apr 14 15:36:55 1989

End Edit History.
*/

#include PACKAGE_SPECS
#include PUBLICTYPES
#include DEVICETYPES
#include DEVIMAGE
#include DEVPATTERN
#include EXCEPT
#include FP
#include PSLIB


#include "imagepriv.h"

/* 2, 4, 8 bit source; 4 bit dest; no halftoning */

#define LOG2BPP 2
#define BPP 4
#define SMASK (SCANMASK >> LOG2BPP)
#define SSHIFT (SCANSHIFT - LOG2BPP)
#define PIXDELTA pixelDelta

public procedure Im113(items, t, run, args)
  integer items;
  DevTrap *t;
  DevRun *run;
  ImageArgs *args; 
  {
  DevMarkInfo *info = args->markInfo;
  DevImage *image = args->image;
  DevImageSource *source = image->source;
  register unsigned char *sLoc;
  register PSCANTYPE pixVals;
  unsigned char *srcLoc;
  DevTrap trap;
  Cd c;
  DevShort *buffptr;
  integer y, ylast, pairs,
    xoffset, yoffset, txl, txr, wbytes, xx, yy, units,
    pixelX, pixelXr, bitr, srcXprv, srcYprv, lastPixel, wb, srcPix;
  DevFixedPoint sv, svy;
  Fixed fxdSrcX, fxdSrcY, fX, fY, srcLX, srcLY, srcGX, srcGY, endX, endY;
  boolean leftSlope, rightSlope;
  PSCANTYPE destunit, destbase;
  SCANTYPE maskl, maskr;
  register integer sPix;
  integer log2BPS;
  integer pixelDelta;
  integer srcMask;
    
  if (args->bitsPerPixel != BPP || args->gray.n != 16)
    RAISE(ecLimitCheck, (char *)NIL);
  
  pixVals = GetPixBuffers();
  
  switch (source->bitspersample) {
    case 2: {log2BPS = 1; srcMask = 3; break; }
    case 4: {log2BPS = 2; srcMask = 1; break; }
    case 8: {log2BPS = 3; srcMask = 0; break; }
    default: RAISE(ecLimitCheck, (char *)NIL);
    }

  PIXDELTA = args->gray.delta;

  /* srcXprv srcYprv are integer coordinates of source pixel */
  srcXprv = image->info.sourcebounds.x.l;
  srcYprv = image->info.sourcebounds.y.l;
  /* srcLX, srcLY, srcGX, srcGY give fixed bounds of source in image space */
  srcLX = Fix(srcXprv);
  srcLY = Fix(srcYprv);
  srcGX = Fix(image->info.sourcebounds.x.g);
  srcGY = Fix(image->info.sourcebounds.y.g);
  /* srcLoc points to source pixel at location (srcXprv, srcYprv) */
  {integer x = srcXprv - image->info.sourceorigin.x;
  srcLoc = source->samples[IMG_GRAY_SAMPLES];
  wbytes = source->wbytes;
  srcLoc += (srcYprv - image->info.sourceorigin.y) * wbytes;
  srcLoc += (x >> (3 - log2BPS));
  srcPix = (x) & srcMask;
  }
  /* image space fixed point vector for (1,0) in device */
  sv.x = pflttofix(&image->info.mtx->a);
  sv.y = pflttofix(&image->info.mtx->b);
  /* image space fixed point vector for (0,1) in device */
  svy.x = pflttofix(&image->info.mtx->c);
  svy.y = pflttofix(&image->info.mtx->d);
  {
  register integer i, g, val;  
  integer nGraysM1 = args->gray.n - 1;
  integer lastPixVal = args->gray.first + nGraysM1 * args->gray.delta; 
  uchar *tfrFcn = (image->transfer == NULL) ? NULL : image->transfer->white;
  switch (log2BPS) {
    case 1: {
      for (i = 0; i < 4; i++) {
        register integer j, m;
	val = i * 85;
	if (tfrFcn != NULL) val = tfrFcn[val];
        val *= nGraysM1;
        g = lastPixVal - (val/255) * PIXDELTA;
        if (val % 255 >= 128) g -= PIXDELTA;
        for (j = 0; j <= 6; j += 2) { /* j is the left shift amount */
          m = i << j; /* byte value after mask */
          pixVals[m] = SHIFTPIXEL(g,0);
          }
	}
      break;
      }
    case 2: {
      for (i = 0; i < 16; i++) {
        register integer j, m;
	val = i * 17;
	if (tfrFcn != NULL) val = tfrFcn[val];
        val *= nGraysM1;
        g = lastPixVal - (val/255) * PIXDELTA;
        if (val % 255 >= 128) g -= PIXDELTA;
        for (j = 0; j <= 4; j += 4) { /* j is the left shift amount */
          m = i << j; /* byte value after mask */
          pixVals[m] = SHIFTPIXEL(g,0);
          }
	}
      break;
      }
    case 3: {
      for (i = 0; i < 256; i++) {
	val = i;
	if (tfrFcn != NULL) val = tfrFcn[val];
        val *= nGraysM1;
        g = lastPixVal - (val/255) * PIXDELTA;
        if (val % 255 >= 128) g -= PIXDELTA;
        pixVals[i] = SHIFTPIXEL(g,0);
	}
      break;
      }
    }
  } /* end of register block */
  xoffset = info->offset.x;
  yoffset = info->offset.y;
  (*args->procs->Begin)(args->data);
  while (--items >= 0) {
    if (run != NULL) {
      y = run->bounds.y.l;
      ylast = run->bounds.y.g - 1;
      buffptr = run->data;
      }
    else if (t != NULL) {
      trap = *t;
      y = trap.y.l;
      ylast = trap.y.g - 1;
      leftSlope = (trap.l.xl != trap.l.xg);
      rightSlope = (trap.g.xl != trap.g.xg);
      txl = trap.l.xl + xoffset;
      txr = trap.g.xl + xoffset;
      if (leftSlope) trap.l.ix += Fix(xoffset);
      if (rightSlope) trap.g.ix += Fix(xoffset);
      }
    else CantHappen();
    y += yoffset; ylast += yoffset;
    /* fX and fY correspond to integer coords (xx, yy) in device space */
    xx = xoffset; yy = y;
    c.x = 0.5; c.y = y - yoffset + 0.5; /* add .5 for center sampling */
    TfmPCd(c, image->info.mtx, &c);
    fX = pflttofix(&c.x); fY = pflttofix(&c.y);
    while (true) {
      pairs = (run != NULL) ? *(buffptr++) : 1;
      while (--pairs >= 0) {
        { register int xl, xr;
          if (run != NULL) {
            xl = *(buffptr++) + xoffset; xr = *(buffptr++) + xoffset; }
          else { xl = txl; xr = txr; }
	  if (xl != xx) { /* update fX, fY for change in device x coord */
            register int diff = xl - xx;
            switch (diff) {
              case  1: fX += sv.x; fY += sv.y; break;
	      case -1: fX -= sv.x; fY -= sv.y; break;
	      default: fX += diff * sv.x; fY += diff * sv.y;
	      }
            xx = xl;
	    }
          if (y != yy) { /* update fX, fY for change in device y coord */
            register int diff = y - yy;
            switch (diff) {
              case  1: fX += svy.x; fY += svy.y; break;
	      default: fX += diff * svy.x; fY += diff * svy.y;
	      }
            yy = y;
	    }
          /* (fX, fY) now gives image space coords for (xl, y) in device */
	  { register integer X, Y;
            X = fX; Y = fY;
	    /* increase xl until (X,Y) is inside bounds of source data */
            until (xl == xr || (
              X >= srcLX && X < srcGX && Y >= srcLY && Y < srcGY)) {
              X += sv.x; Y += sv.y; xl++;
              }
            fxdSrcX = X; fxdSrcY = Y;
	    /* (fxdSrcX, fxdSrcY) = image coords for (xl, y) in device */
            X = X + sv.x * (xr - xl - 1);
            Y = Y + sv.y * (xr - xl - 1);
	    /* X Y now gives image coords for (xr-1, y) */
	    /* decrease xr until last pixel is inside bounds of source data */
            until (xl == xr || (
              X >= srcLX && X < srcGX && Y >= srcLY && Y < srcGY)) {
              X -= sv.x; Y -= sv.y; xr--;
              }
            endX = X; endY = Y;
	    /* (endX, endY) = image coords for (xr-1, y) in device */
            }
          if (xl == xr) continue; /* nothing left for this pair */
          destbase = (*args->procs->GetWriteScanline)(args->data, y, xl, xr);
          pixelX = (xl & SMASK) << LOG2BPP;
          pixelXr = (xr & SMASK) << LOG2BPP;
          { register int unitl;
            unitl = xl >> SSHIFT;
            destunit = destbase + unitl;
            units = (xr >> SSHIFT) - unitl;
            }
          }
        maskl = leftBitArray[pixelX];
        if (pixelXr == 0) {
	  units--; pixelXr = SCANUNIT; maskr = -1; bitr = SCANUNIT;
	  }
        else {
	  bitr = pixelXr; maskr = rightBitArray[bitr];
	  };
	lastPixel = (units > 0) ? SCANUNIT : pixelXr;
	sLoc = srcLoc;
        { register int sY;
          sY = IntPart(fxdSrcY);
          if (sY != srcYprv) {
            register int diff;
            switch (diff = sY - srcYprv) {
              case  1: sLoc += wbytes; break;
              case -1: sLoc -= wbytes; break;
              default: sLoc += diff * wbytes;
              }
            srcYprv = sY;
            }
          }
	sPix = srcPix;
        { register int sX;
          sX = IntPart(fxdSrcX);
          if (sX != srcXprv) {
            register int diff;
	    diff = sX - srcXprv;
	    switch (log2BPS) {
	      case 1: {
		if (diff > 0) {
		  sLoc += (diff >> 2); sPix += diff & 3;
		  if (sPix > 3) { sPix -= 4; sLoc++; }
		  }
		else {
		  diff = - diff;
		  sLoc -= (diff >> 2); sPix -= diff & 3;
		  if (sPix < 0) { sPix += 4; sLoc--; }
		  };
        	srcPix = sPix;
		break;
		}
	      case 2: {
		if (diff > 0) {
		  sLoc += (diff >> 1); sPix += diff & 1;
		  if (sPix > 1) { sPix -= 2; sLoc++; }
		  }
		else {
		  diff = - diff;
		  sLoc -= (diff >> 1); sPix -= diff & 1;
		  if (sPix < 0) { sPix += 2; sLoc--; }
		  };
        	srcPix = sPix;
		break;
		}
	      case 3: {
		sLoc += diff;
		break;
		}
	      }
            srcXprv = sX;
            }
          }
	  
        srcLoc = sLoc;
        
	if (IntPart(fxdSrcX) == IntPart(endX)) { /* vertical source */
          register int b;
          register SCANTYPE temp;
          register Fixed ey, dy;
          register integer sData;
	  register boolean needData;
          temp = *destunit & ~maskl;
	  needData = true;
	  if (sv.y > 0) {
            dy = sv.y; wb = wbytes; ey = ONE - (fxdSrcY & LowMask); }
          else {
            dy = -sv.y; wb = -wbytes; ey = (fxdSrcY & LowMask) + 1; }
          switch (log2BPS) {
            case 1: { 
              register unsigned char sMsk = 0xC0 >> (sPix << 1);
	      while (true) { /* units loop */
		b = lastPixel - pixelX - BPP;
		do { /* pixelX loop */
		  if (needData) {
		    needData = false;
		    sData = pixVals[*sLoc & sMsk];
		    };
	          temp |= sData LSHIFT b;
		  ey -= dy;
		  if (ey <= 0) {
		    do sLoc += wb; while ((ey += ONE) <= 0);
		    needData = true;
		    }
		  } while ((b -= BPP) >= 0); /* end of pixel loop */
		switch (--units) {
		  case 0: lastPixel = pixelXr; break;
		  case -1:
		    if (pixelXr == SCANUNIT) *destunit = temp;
		    else {
		      temp LSHIFTEQ (SCANUNIT - bitr);
		      if (pixelX > 0) {
			temp &= maskl;
			temp |= *destunit & ~maskl;
			}
		      *destunit = temp | (*destunit & ~maskr);
		      }
		    goto NextPair;
		  }
		*destunit++ = temp;
		temp = 0; pixelX = 0;
		} /* end of units loop */
	      }
            case 2: { 
              register unsigned char sMsk = 0xF0 >> (sPix << 2);
	      while (true) { /* units loop */
		b = lastPixel - pixelX - BPP;
		do { /* pixelX loop */
		  if (needData) {
		    needData = false;
		    sData = pixVals[*sLoc & sMsk];
		    };
	          temp |= sData LSHIFT b;
		  ey -= dy;
		  if (ey <= 0) {
		    do sLoc += wb; while ((ey += ONE) <= 0);
		    needData = true;
		    }
		  } while ((b -= BPP) >= 0); /* end of pixel loop */
		switch (--units) {
		  case 0: lastPixel = pixelXr; break;
		  case -1:
		    if (pixelXr == SCANUNIT) *destunit = temp;
		    else {
		      temp LSHIFTEQ (SCANUNIT - bitr);
		      if (pixelX > 0) {
			temp &= maskl;
			temp |= *destunit & ~maskl;
			}
		      *destunit = temp | (*destunit & ~maskr);
		      }
		    goto NextPair;
		  }
		*destunit++ = temp;
		temp = 0; pixelX = 0;
		} /* end of units loop */
	      }
            case 3: { 
	      while (true) { /* units loop */
		b = lastPixel - pixelX - BPP;
		do { /* pixelX loop */
		  if (needData) {
		    needData = false;
		    sData = pixVals[*sLoc];
		    };
	          temp |= sData LSHIFT b;
		  ey -= dy;
		  if (ey <= 0) {
		    do sLoc += wb; while ((ey += ONE) <= 0);
		    needData = true;
		    }
		  } while ((b -= BPP) >= 0); /* end of pixel loop */
		switch (--units) {
		  case 0: lastPixel = pixelXr; break;
		  case -1:
		    if (pixelXr == SCANUNIT) *destunit = temp;
		    else {
		      temp LSHIFTEQ (SCANUNIT - bitr);
		      if (pixelX > 0) {
			temp &= maskl;
			temp |= *destunit & ~maskl;
			}
		      *destunit = temp | (*destunit & ~maskr);
		      }
		    goto NextPair;
		  }
		*destunit++ = temp;
		temp = 0; pixelX = 0;
		} /* end of units loop */
	      }
            }
          goto NextPair;
          } /* end vert case */
	else if (IntPart(fxdSrcY) == IntPart(endY)) { /* horizontal source */
          register int b;
          register SCANTYPE temp;
          register Fixed ex, dx;
          register integer sData;
	  register boolean needData;
          temp = *destunit & ~maskl;
	  needData = true;
	  dx = sv.x;
	  if (sv.x > 0)
            ex = ONE - (fxdSrcX & LowMask);
          else 
            ex = (fxdSrcX & LowMask) + 1;
          switch (log2BPS) {
            case 1: { 
              register unsigned char sMsk = 0xC0 >> (sPix << 1);
	      while (true) { /* units loop */
		b = lastPixel - pixelX - BPP;
		if (dx > 0) {
		  do { /* pixelX loop */
		    if (needData) {
		      needData = false;
		      sData = pixVals[*sLoc & sMsk];
		      };
	            temp |= sData LSHIFT b;
		    ex -= dx;
		    if (ex <= 0) {
		      do {
		        sMsk >>= 2;
		        if (sMsk == 0) { ++sLoc; sMsk = 0xC0; }
		        } while ((ex += ONE) <= 0);
		      needData = true;
		      }
		    } while ((b -= BPP) >= 0); /* end of pixel loop */
		  }
		else { /* dx <= 0 */
		  do { /* pixelX loop */
		    if (needData) {
		      needData = false;
		      sData = pixVals[*sLoc & sMsk];
		      };
	            temp |= sData LSHIFT b;
		    ex += dx;
		    if (ex <= 0) {
		      do {
		        if (sMsk == 0xC0) { --sLoc; sMsk = 0x03; }
		        else sMsk <<= 2;
		        } while ((ex += ONE) <= 0);
		      needData = true;
		      }
		    } while ((b -= BPP) >= 0); /* end of pixel loop */
		  }
		switch (--units) {
		  case 0: lastPixel = pixelXr; break;
		  case -1:
		    if (pixelXr == SCANUNIT) *destunit = temp;
		    else {
		      temp LSHIFTEQ (SCANUNIT - bitr);
		      if (pixelX > 0) {
			temp &= maskl;
			temp |= *destunit & ~maskl;
			}
		      *destunit = temp | (*destunit & ~maskr);
		      }
		    goto NextPair;
		  }
		*destunit++ = temp;
		temp = 0; pixelX = 0;
		} /* end of units loop */
	      }
            case 2: { 
              register unsigned char sMsk = 0xF0 >> (sPix << 2);
	      while (true) { /* units loop */
		b = lastPixel - pixelX - BPP;
		if (dx > 0) {
		  do { /* pixelX loop */
		    if (needData) {
		      needData = false;
		      sData = pixVals[*sLoc & sMsk];
		      };
	            temp |= sData LSHIFT b;
		    ex -= dx;
		    if (ex <= 0) {
		      do {
		        sMsk >>= 4;
		        if (sMsk == 0) { ++sLoc; sMsk = 0xF0; }
		        } while ((ex += ONE) <= 0);
		      needData = true;
		      }
		    } while ((b -= BPP) >= 0); /* end of pixel loop */
		  }
		else { /* dx <= 0 */
		  do { /* pixelX loop */
		    if (needData) {
		      needData = false;
		      sData = pixVals[*sLoc & sMsk];
		      };
	            temp |= sData LSHIFT b;
		    ex += dx;
		    if (ex <= 0) {
		      do {
		        if (sMsk == 0xF0) {sLoc--; sMsk = 0x0F; }
		        else sMsk <<= 4;
		        } while ((ex += ONE) <= 0);
		      needData = true;
		      }
		    } while ((b -= BPP) >= 0); /* end of pixel loop */
		  }
		switch (--units) {
		  case 0: lastPixel = pixelXr; break;
		  case -1:
		    if (pixelXr == SCANUNIT) *destunit = temp;
		    else {
		      temp LSHIFTEQ (SCANUNIT - bitr);
		      if (pixelX > 0) {
			temp &= maskl;
			temp |= *destunit & ~maskl;
			}
		      *destunit = temp | (*destunit & ~maskr);
		      }
		    goto NextPair;
		  }
		*destunit++ = temp;
		temp = 0; pixelX = 0;
		} /* end of units loop */
	      }
            case 3: { 
	      while (true) { /* units loop */
		b = lastPixel - pixelX - BPP;
		if (dx > 0) {
		  do { /* pixelX loop */
		    if (needData) {
		      needData = false;
		      sData = pixVals[*sLoc];
		      };
	            temp |= sData LSHIFT b;
		    ex -= dx;
		    if (ex <= 0) {
		      do sLoc++; while ((ex += ONE) <= 0);
		      needData = true; 
		      }
		    } while ((b -= BPP) >= 0); /* end of pixel loop */
		  }
		else { /* dx <= 0 */
		  do { /* pixelX loop */
		    if (needData) {
		      needData = false;
		      sData = pixVals[*sLoc];
		      };
	            temp |= sData LSHIFT b;
		    ex += dx;
		    if (ex <= 0) {
		      do sLoc--; while ((ex += ONE) <= 0);
		      needData = true;
		      }
		    } while ((b -= BPP) >= 0); /* end of pixel loop */
		  }
		switch (--units) {
		  case 0: lastPixel = pixelXr; break;
		  case -1:
		    if (pixelXr == SCANUNIT) *destunit = temp;
		    else {
		      temp LSHIFTEQ (SCANUNIT - bitr);
		      if (pixelX > 0) {
			temp &= maskl;
			temp |= *destunit & ~maskl;
			}
		      *destunit = temp | (*destunit & ~maskr);
		      }
		    goto NextPair;
		  }
		*destunit++ = temp;
		temp = 0; pixelX = 0;
		} /* end of units loop */
	      }
            }  
          goto NextPair;
          } /* end horiz case */
	else { /* general rotation case */
          register int b;
          register SCANTYPE temp;
          register Fixed ex, ey;
          register integer sData;
	  register Fixed dx, dy;
	  register boolean needData;
          temp = *destunit & ~maskl;
	  needData = true;
	  dx = sv.x; 
	  if (sv.x > 0) 
            ex = ONE - (fxdSrcX & LowMask);
          else 
            ex = (fxdSrcX & LowMask) + 1;
	  if (sv.y > 0) {
            dy = sv.y; wb = wbytes; ey = ONE - (fxdSrcY & LowMask); }
          else {
            dy = -sv.y; wb = -wbytes; ey = (fxdSrcY & LowMask) + 1; }
          switch (log2BPS) {
            case 1: { 
	      register unsigned char sMsk = 0xC0 >> (sPix << 1);
	      while (true) { /* units loop */
		b = lastPixel - pixelX - BPP;
		if (dx > 0) {
		  do { /* pixelX loop */
		    if (needData) {
		      needData = false;
		      sData = pixVals[*sLoc & sMsk];
		      };
	            temp |= sData LSHIFT b;
		    ey -= dy;
		    if (ey <= 0) {
		      do sLoc += wb; while ((ey += ONE) <= 0);
	              needData = true;
		      }
		    ex -= dx;
		    if (ex <= 0) {
		      do {
                        sMsk >>= 2;
		        if (sMsk == 0) { sLoc++; sMsk = 0xC0; }
		        } while ((ex += ONE) <= 0);
		      needData = true;
		      }
		    } while ((b -= BPP) >= 0); /* end of pixel loop */
		  }
		else { /* dx <= 0 */
		  do { /* pixelX loop */
		    if (needData) {
		      needData = false;
		      sData = pixVals[*sLoc & sMsk];
		      };
	            temp |= sData LSHIFT b;
		    ey -= dy;
		    if (ey <= 0) {
		      do sLoc += wb; while ((ey += ONE) <= 0);
	              needData = true;
		      }
		    ex += dx;
		    if (ex <= 0) {
		      do {
		        if (sMsk == 0xC0) { --sLoc; sMsk = 0x03; }
		        else sMsk <<= 2;
		        } while ((ex += ONE) <= 0);
		      needData = true;
		      } 
		    } while ((b -= BPP) >= 0); /* end of pixel loop */
		  }
		switch (--units) {
		  case 0: lastPixel = pixelXr; break;
		  case -1:
		    if (pixelXr == SCANUNIT) *destunit = temp;
		    else {
		      temp LSHIFTEQ (SCANUNIT - bitr);
		      if (pixelX > 0) {
			temp &= maskl;
			temp |= *destunit & ~maskl;
			}
		      *destunit = temp | (*destunit & ~maskr);
		      }
		    goto NextPair;
		  }
		*destunit++ = temp;
		temp = 0; pixelX = 0;
		} /* end of units loop */
	      }
            case 2: { 
              register unsigned char sMsk = 0xF0 >> (sPix << 2);
	      while (true) { /* units loop */
		b = lastPixel - pixelX - BPP;
		if (dx > 0) {
		  do { /* pixelX loop */
		    if (needData) {
		      needData = false;
		      sData = pixVals[*sLoc & sMsk];
		      };
	            temp |= sData LSHIFT b;
		    ey -= dy;
		    if (ey <= 0) {
		      do sLoc += wb; while ((ey += ONE) <= 0);
	              needData = true;
		      }
		    ex -= dx;
		    if (ex <= 0) {
		      do {
		        sMsk >>= 4;
		        if (sMsk == 0) { ++sLoc; sMsk = 0xF0; }
		        } while ((ex += ONE) <= 0);
		      needData = true;
		      }
		    } while ((b -= BPP) >= 0); /* end of pixel loop */
		  }
		else { /* dx <= 0 */
		  do { /* pixelX loop */
		    if (needData) {
		      needData = false;
		      sData = pixVals[*sLoc & sMsk];
		      };
	            temp |= sData LSHIFT b;
		    ey -= dy;
		    if (ey <= 0) {
		      do sLoc += wb; while ((ey += ONE) <= 0);
	              needData = true;
		      }
		    ex += dx;
		    if (ex <= 0) {
		      do {
		        if (sMsk == 0xF0) {sLoc--; sMsk = 0x0F; }
		        else sMsk <<= 4;
		        } while ((ex += ONE) <= 0);
		      needData = true;
		      }
		    } while ((b -= BPP) >= 0); /* end of pixel loop */
		  }
		switch (--units) {
		  case 0: lastPixel = pixelXr; break;
		  case -1:
		    if (pixelXr == SCANUNIT) *destunit = temp;
		    else {
		      temp LSHIFTEQ (SCANUNIT - bitr);
		      if (pixelX > 0) {
			temp &= maskl;
			temp |= *destunit & ~maskl;
			}
		      *destunit = temp | (*destunit & ~maskr);
		      }
		    goto NextPair;
		  }
		*destunit++ = temp;
		temp = 0; pixelX = 0;
		} /* end of units loop */
	      }
            case 3: { 
	      while (true) { /* units loop */
		b = lastPixel - pixelX - BPP;
		if (dx > 0) {
		  do { /* pixelX loop */
		    if (needData) {
		      needData = false;
		      sData = pixVals[*sLoc];
		      };
	            temp |= sData LSHIFT b;
		    ey -= dy;
		    if (ey <= 0) {
		      do sLoc += wb; while ((ey += ONE) <= 0);
	              needData = true;
		      }
		    ex -= dx;
		    if (ex <= 0) {
		      do sLoc++; while ((ex += ONE) <= 0);
		      needData = true;
		      }
		    } while ((b -= BPP) >= 0); /* end of pixel loop */
		  }
		else { /* dx <= 0 */
		  do { /* pixelX loop */
		    if (needData) {
		      needData = false;
		      sData = pixVals[*sLoc];
		      };
	            temp |= sData LSHIFT b;
		    ey -= dy;
		    if (ey <= 0) {
		      do sLoc += wb; while ((ey += ONE) <= 0);
	              needData = true;
		      }
		    ex += dx;
		    if (ex <= 0) {
		      do sLoc--; while ((ex += ONE) <= 0);
		      needData = true;
		      }
		    } while ((b -= BPP) >= 0); /* end of pixel loop */
		  }
		switch (--units) {
		  case 0: lastPixel = pixelXr; break;
		  case -1:
		    if (pixelXr == SCANUNIT) *destunit = temp;
		    else {
		      temp LSHIFTEQ (SCANUNIT - bitr);
		      if (pixelX > 0) {
			temp &= maskl;
			temp |= *destunit & ~maskl;
			}
		      *destunit = temp | (*destunit & ~maskr);
		      }
		    goto NextPair;
		  }
		*destunit++ = temp;
		temp = 0; pixelX = 0;
		} /* end of units loop */
	      }
            }  
          }
        NextPair: continue;
        }
      if (++y > ylast) break;
      if (t != NULL) {
        if (y == ylast) {
          txl = trap.l.xg + xoffset;
          txr = trap.g.xg + xoffset;
          }
        else {
          if (leftSlope) { txl = IntPart(trap.l.ix); trap.l.ix += trap.l.dx; }
          if (rightSlope) { txr = IntPart(trap.g.ix); trap.g.ix += trap.g.dx; }
          }
        }
      }
    if (t != NULL) t++;
    }
  (*args->procs->End)(args->data);
  } /* end ImS1XD1X */

