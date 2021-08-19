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
Ed Taft: Tue Jul  3 12:55:59 1984
Ivor Durham: Sun Feb  7 14:02:56 1988
Bill Paxton: Tue Mar 29 09:20:13 1988
Paul Rovner: Monday, November 23, 1987 10:11:58 AM
Jim Sandman: Wed Aug  2 12:18:52 1989
End Edit History.
*/

#include PACKAGE_SPECS
#include PUBLICTYPES
#include DEVIMAGE
#include DEVPATTERN
#include EXCEPT

#include "imagepriv.h"

#define MAXCOLOR 255

public procedure Im106(items, t, run, args)
  /* 1 bit per pixel source ; SCANUNIT bits per dest pixel; */
  integer items;
  DevTrap *t;
  DevRun *run;
  ImageArgs *args; 
  {
  DevMarkInfo info;
  DevImage *image = args->image;
  DevImageSource *source = image->source;
  DevTrap trap;
  DevShort *buffptr;
  integer y, xl, xr, ylast, scrnRow, pairs, xoffset, yoffset;
  integer txl, txr, xx, yy;
  Fixed fX, fY, fxdSrcX, fxdSrcY, srcLX, srcLY, srcGX, srcGY, endX, endY;
  DevFixedPoint sv, svy;
  boolean leftSlope, rightSlope, isMask;
  PSCANTYPE destbase;
  integer srcX, srcY, diff, srcXprv, srcYprv, k, m;
  Cd c;
  integer wbytes, srcBit;
  uchar foreground, background;
  PatternData data;
  DevColorVal color;
  SCANTYPE foreValue, backValue;
  uchar *tfrGry, *srcLoc;
  boolean invert;
  info = *args->markInfo;
  if (args->bitsPerPixel != SCANUNIT)
    RAISE(ecLimitCheck, (char *)NIL);
  
  /* the following source is not dependent on bits per pixel.
     Appropriate macros have been defined to remove dependencies. */
  
  /* srcXprv srcYprv are integer coordinates of source pixel */
  srcXprv = image->info.sourcebounds.x.l;
  srcYprv = image->info.sourcebounds.y.l;
  /* srcLX, srcLY, srcGX, srcGY give fixed bounds of source in image space */
  srcLX = Fix(srcXprv);
  srcLY = Fix(srcYprv);
  srcGX = Fix(image->info.sourcebounds.x.g);
  srcGY = Fix(image->info.sourcebounds.y.g);
  /* srcLoc points to source pixel at location (srcXprv, srcYprv) */
  srcLoc = source->samples[IMG_GRAY_SAMPLES];
  wbytes = source->wbytes;
  srcLoc += (srcYprv - image->info.sourceorigin.y) * wbytes;
  srcLoc += ((srcXprv - image->info.sourceorigin.x) >> 3);
  srcBit = (srcXprv - image->info.sourceorigin.x) & 7;
  /* image space fixed point vector for (1,0) in device */
  sv.x = pflttofix(&image->info.mtx->a);
  sv.y = pflttofix(&image->info.mtx->b);
  /* image space fixed point vector for (0,1) in device */
  svy.x = pflttofix(&image->info.mtx->c);
  svy.y = pflttofix(&image->info.mtx->d);

  isMask = image->imagemask;
  tfrGry = (image->transfer == NULL) ? NULL : image->transfer->white;
  xoffset = info.offset.x;
  yoffset = info.offset.y;
  
  (*args->procs->Begin)(args->data);
  
  if (isMask) { /* imagemask operator */
    invert = !(image->invert);
    }
  else { /* 1 bit per pixel image */
    if (tfrGry == NULL) {
      background = MAXCOLOR;
      foreground = 0;
      invert = true;
      }
    else {
      if (tfrGry[MAXCOLOR] == 255) {
	background = 255;
	foreground = tfrGry[0];
	invert = true;
	}
      else if (tfrGry[0] == 255) {
	background = 255;
	foreground = tfrGry[MAXCOLOR];
	invert = false;
	}
      else if (tfrGry[MAXCOLOR] == 0) {
	background = 0;
	foreground = tfrGry[0];
	invert = true;
	}
      else {
	background = tfrGry[0];
	foreground = tfrGry[MAXCOLOR];
	invert = false;
	}
      }
    color.red = color.green = color.blue = color.white = background;
    info.color = *((DevColor *)&color);
    SetupPattern(args->pattern, &info, &data);
    if (!data.constant)
      RAISE(ecLimitCheck, (char *)NIL);
    backValue = data.value;
    color.red = color.green = color.blue = color.white = foreground;
    info.color = *((DevColor *)&color);
    }
    
  SetupPattern(args->pattern, &info, &data);
  if (!data.constant)
    RAISE(ecLimitCheck, (char *)NIL);
  foreValue = data.value;
  
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

    xx = xoffset; yy = y;
    c.x = 0.5; c.y = y - yoffset + 0.5; /* add .5 for center sampling */
    TfmPCd(c, image->info.mtx, &c);
    fX = pflttofix(&c.x); fY = pflttofix(&c.y);
    while (true) {
      pairs = (run != NULL) ? *(buffptr++) : 1;
      while (--pairs >= 0) {
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
        if (isMask)
          destbase =
            (*args->procs->GetReadWriteScanline)(args->data, y, xl, xr);
        else { /* paint xl to xr with background color */
          register PSCANTYPE destunit;
	  register int units;
          destbase = (*args->procs->GetWriteScanline)(args->data, y, xl, xr);
          destunit = destbase + xl;
          units = xr - xl;
          while (units--) *(destunit++) = backValue;
          }
        { /* paint foreground color */
          register uchar *sLoc = srcLoc;
          register PSCANTYPE destunit = destbase + xl;
	  register int units;
	  units = xr - xl;
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
        { register int sX;
          sX = IntPart(fxdSrcX);
          if (sX != srcXprv) {
            register int diff, sBit = srcBit;
            switch (diff = sX - srcXprv) {
              case -1: if (--sBit < 0) { sBit = 7; sLoc--; } break;
              case  1: if (++sBit > 7) { sBit = 0; sLoc++; } break;
              default:
                if (diff > 0) {
                  sLoc += diff >> 3; sBit += diff & 7;
                  if (sBit > 7) { sBit -= 8; sLoc++; }
                  }
                else {
                  diff = -diff; sLoc -= diff >> 3; sBit -= diff & 7;
                  if (sBit < 0) { sBit += 8; sLoc--; }
                  }
              }
            srcXprv = sX; srcBit = sBit;
            }
          }
        srcLoc = sLoc;
	if (IntPart(fxdSrcX) == IntPart(endX)) { /* vertical source */
	  register Fixed ey, dy;
	  register unsigned char sData, sInvert;
	  register unsigned char sBit = 0x80 >> srcBit;
          register boolean needData;
          register int wb;
	  register Fixed one = ONE;
	  if (sv.y > 0) {
	    dy = sv.y; wb = wbytes; ey = one - (fxdSrcY & LowMask); }
	  else {
	    dy = -sv.y; wb = -wbytes; ey = (fxdSrcY & LowMask) + 1; }
          sInvert = invert ? 0xFF : 0;
          needData = true;
	  while (units--) { /* units loop */
	    if (needData) {
	      needData = false;
	      sData = *sLoc ^ sInvert;
	      };
	    if ((sData & sBit) != 0) 
	      *destunit = foreValue;
	    destunit++;
	    ey -= dy;
	    if (ey <= 0) {
	      do sLoc += wb; while ((ey += one) <= 0);
	      needData = true;
	      }
	    } /* end of units loop */
	  } /* end vertical source */
	else if (IntPart(fxdSrcY) == IntPart(endY)) { /* horizontal source */
	  register Fixed ex, dx;
	  register unsigned char sData, sInvert;
	  register unsigned char sBit = 0x80 >> srcBit;
          register boolean needData;
          sInvert = invert ? 0xFF : 0;
	  needData = true;
	  dx = sv.x;
	  if (sv.x > 0)
	    ex = ONE - (fxdSrcX & LowMask);
	  else
	    ex = (fxdSrcX & LowMask) + 1;
	  if (dx > 0) {
	    while (units--) { /* units loop */
	      if (needData) {
		needData = false;
		sData = *sLoc ^ sInvert;
		};
	      if ((sData & sBit) != 0) 
		*destunit = foreValue;
	      destunit++;
	      ex -= dx;
	      if (ex <= 0) {
		do {
		  sBit >>= 1;
		  if (sBit == 0) {
		    sBit = 0x80; sLoc++;
		    }
		  } while ((ex += ONE) <= 0);
		needData = true;
		}
	      } /* end of units loop */
	    }
	  else { /* dx <= 0 */
	    while (units--) { 
	      if (needData) {
		sData = *sLoc ^ sInvert;  needData = false;
		}
	      if ((sData & sBit) != 0) 
		*destunit = foreValue;
	      destunit++;
	      ex += dx;
	      if (ex <= 0) {
		do {
		  if (sBit == 0x80) {
		    sBit = 1; sLoc--;
		    }
		  else sBit <<= 1;
		  } while ((ex += ONE) <= 0);
		needData = true;
		}
	      }
            }
	  } /* end horiz case */
	else { /* general rotation case */
          register unsigned char *dx, *dy;
          register unsigned char sData, sInvert;
	  register unsigned char sBit = 0x80 >> srcBit;
          register Fixed ex, ey;
          register uchar *wb;
          register boolean needData;
          dx = (uchar *)sv.x;
          if (sv.y > 0) {
            dy = (uchar *)sv.y;
            wb = (uchar *)wbytes;
            ey = ONE - (fxdSrcY & LowMask);
            }
          else {
            dy = (uchar *)-sv.y;
            wb = (uchar *)-wbytes;
            ey = (fxdSrcY & LowMask) + 1;
            }
          ex = ((Fixed)dx > 0) ? ONE - (fxdSrcX & LowMask) :
                                 (fxdSrcX & LowMask) + 1;
          sInvert = invert ? 0xFF : 0;
          sData = *sLoc ^ sInvert;
          needData = false;
	  if ((Fixed)dx > 0) {
	    while (units--) { 
	      if (needData) {
		sData = *sLoc ^ sInvert;  needData = false;
		}
	      if ((sData & sBit) != 0) 
		*destunit = foreValue;
	      destunit++;
	      ey -= (Fixed)dy;
	      if (ey <= 0) {
		do sLoc += (integer)wb; while ((ey += ONE) <= 0);
		needData = true;
		}
	      ex -= (Fixed)dx;
	      if (ex <= 0) {
		do {
		  sBit >>= 1;
		  if (sBit == 0) {
		    sBit = 0x80; sLoc++;
		    }
		  } while ((ex += ONE) <= 0);
		needData = true;
		}
	      }
	    }
	  else { /* dx <= 0 */
	    while (units--) { 
	      if (needData) {
		sData = *sLoc ^ sInvert;  needData = false;
		}
	      if ((sData & sBit) != 0) 
		*destunit = foreValue;
	      destunit++;
	      ey -= (Fixed)dy;
	      if (ey <= 0) {
		do sLoc += (integer)wb; while ((ey += ONE) <= 0);
		needData = true;
		}
	      ex += (Fixed)dx;
	      if (ex <= 0) {
		do {
		  if (sBit == 0x80) {
		    sBit = 1; sLoc--;
		    }
		  else sBit <<= 1;
		  } while ((ex += ONE) <= 0);
		needData = true;
		}
	      }
            }
          }
         }
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
  }

