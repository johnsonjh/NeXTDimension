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
Ed Taft: Wed Dec 13 18:17:49 1989
Ivor Durham: Wed Mar 23 13:11:32 1988
Bill Paxton: Tue Mar 29 09:25:41 1988
Paul Rovner: Wed Aug 23 11:42:35 1989
Jim Sandman: Wed Feb  1 10:07:41 1989

End Edit History.
*/

#include PACKAGE_SPECS
#include PUBLICTYPES
#include DEVICETYPES
#include DEVIMAGE
#include DEVPATTERN
#include EXCEPT

#include "imagepriv.h"

private integer twoBitSample[4] = {255, 170, 85, 0};
private integer fourBitSample[16] = {255, 238, 221, 204, 187, 170, 153, 136, 119, 102, 85, 68, 51, 34, 17, 0};

typedef enum {
  s11, s21, s41, s81, s13I, s23I, s43I, s83I, s14I, s24I, s44I, s84I,
  s13N, s23N, s43N, s83N, s14N, s24N, s44N, s84N, sBad
  } ImSourceType;

 
public procedure Im116(items, t, run, args)
  integer items;
  DevTrap *t;
  DevRun *run;
  ImageArgs *args; 
  {
  DevMarkInfo *info = args->markInfo;
  DevImage *image = args->image;
  DevImageSource *source = image->source;
  DevTrap trap;
  Cd c;
  private PSCANTYPE cyanV, magentaV, yellowV, blackV;
  integer data0, data1, data2, data3;
  integer black, removal;
  integer y, ylast, pairs, wbytes,
    xoffset, yoffset, txl, txr, xx, yy, units,
    bitr, srcXprv, srcYprv, lastPixel;
  DevShort *buffptr;
  DevFixedPoint sv, svy;
  Fixed fxdSrcX, fxdSrcY, fX, fY, srcLX, srcLY, srcGX, srcGY, endX, endY;
  boolean leftSlope, rightSlope;
  PSCANTYPE destunit, destbase;
  ImSourceType sourceType = sBad;
  PCard8 tfrBG;  DevShort *tfrUCR;

  wbytes = source->wbytes;
  
  if (args->bitsPerPixel != SCANUNIT)
    RAISE(ecLimitCheck, (char *)NIL);

  cyanV = GetRedPixBuffers();
  magentaV = GetGreenPixBuffers();
  yellowV = GetBluePixBuffers();
  blackV = GetWhitePixBuffers();
  
  {
  PCard8 tfrRed, tfrGreen, tfrBlue, tfrWhite;
  PVoidProc setValuesProc;
  integer i;
  if (image->transfer == NULL) {
    tfrRed = tfrGreen = tfrBlue = tfrWhite = tfrBG = NULL;
	tfrUCR = (DevShort *)NULL;
  }
  else {
    tfrRed = image->transfer->red;
    tfrGreen = image->transfer->green;
    tfrBlue = image->transfer->blue;
    tfrWhite = image->transfer->white;
    tfrBG = image->transfer->bg;
    tfrUCR = image->transfer->ucr;
    };
  if (source->colorSpace == DEVRGB_COLOR_SPACE) {
    setValuesProc = SetRGBtoCMYKValues;
    }
  else
    switch (source->bitspersample) {
      case 1:
	setValuesProc = Set1BitCMYKValues;
	break;
      case 2:
	setValuesProc = Set2BitCMYKValues;
	break;
      case 4:
	setValuesProc = Set4BitCMYKValues;
	break;
      case 8:
	setValuesProc = Set8BitCMYKValues;
	break;
      }
    (*setValuesProc)(args->red, tfrRed, cyanV, &source->decode[IMG_CYAN_SAMPLES]);
    (*setValuesProc)(args->green, tfrGreen, magentaV, &source->decode[IMG_MAGENTA_SAMPLES]);
    (*setValuesProc)(args->blue, tfrBlue, yellowV, &source->decode[IMG_YELLOW_SAMPLES]);
    (*setValuesProc)(args->gray, tfrWhite, blackV, &source->decode[IMG_BLACK_SAMPLES]);
  }
  /* srcXprv srcYprv are integer coordinates of source pixel */
  srcXprv = image->info.sourcebounds.x.l;
  srcYprv = image->info.sourcebounds.y.l;
  /* srcLX, srcLY, srcGX, srcGY give fixed bounds of source in image space */
  srcLX = Fix(srcXprv);
  srcLY = Fix(srcYprv);
  srcGX = Fix(image->info.sourcebounds.x.g);
  srcGY = Fix(image->info.sourcebounds.y.g);
  /* image space fixed point vector for (1,0) in device */
  sv.x = pflttofix(&image->info.mtx->a);
  sv.y = pflttofix(&image->info.mtx->b);
  /* image space fixed point vector for (0,1) in device */
  svy.x = pflttofix(&image->info.mtx->c);
  svy.y = pflttofix(&image->info.mtx->d);
  /* if this is null then use raw source data */
  xoffset = info->offset.x;
  yoffset = info->offset.y;
  switch (source->bitspersample) {
    case 1:
      if (source->interleaved) {
        switch (source->colorSpace) {
          case DEVGRAY_COLOR_SPACE: sourceType = s11; break;
          case DEVRGB_COLOR_SPACE: sourceType = s13I; break;
          case DEVCMYK_COLOR_SPACE: sourceType = s14I; break;
          }
        }
      else {
        switch (source->colorSpace) {
          case DEVGRAY_COLOR_SPACE: sourceType = s11; break;
          case DEVRGB_COLOR_SPACE: sourceType = s13N; break;
          case DEVCMYK_COLOR_SPACE: sourceType = s14N; break;
          }
        }
      break;
    case 2:
      if (source->interleaved) {
        switch (source->colorSpace) {
          case DEVGRAY_COLOR_SPACE: sourceType = s21; break;
          case DEVRGB_COLOR_SPACE: sourceType = s23I; break;
          case DEVCMYK_COLOR_SPACE: sourceType = s24I; break;
          }
        }
      else {
        switch (source->colorSpace) {
          case DEVGRAY_COLOR_SPACE: sourceType = s21; break;
          case DEVRGB_COLOR_SPACE: sourceType = s23N; break;
          case DEVCMYK_COLOR_SPACE: sourceType = s24N; break;
          }
        }
      break;
    case 4:
      if (source->interleaved) {
        switch (source->colorSpace) {
          case DEVGRAY_COLOR_SPACE: sourceType = s41; break;
          case DEVRGB_COLOR_SPACE: sourceType = s43I; break;
          case DEVCMYK_COLOR_SPACE: sourceType = s44I; break;
          }
        }
      else {
        switch (source->colorSpace) {
          case DEVGRAY_COLOR_SPACE: sourceType = s41; break;
          case DEVRGB_COLOR_SPACE: sourceType = s43N; break;
          case DEVCMYK_COLOR_SPACE: sourceType = s44N; break;
          }
        }
      break;
    case 8:
      if (source->interleaved) {
        switch (source->colorSpace) {
          case DEVGRAY_COLOR_SPACE: sourceType = s81; break;
          case DEVRGB_COLOR_SPACE: sourceType = s83I; break;
          case DEVCMYK_COLOR_SPACE: sourceType = s84I; break;
          }
        }
      else {
        switch (source->colorSpace) {
          case DEVGRAY_COLOR_SPACE: sourceType = s81; break;
          case DEVRGB_COLOR_SPACE: sourceType = s83N; break;
          case DEVCMYK_COLOR_SPACE: sourceType = s84N; break;
          }
        }
      break;
    }
  if (sourceType == sBad)
    CantHappen();
      
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
          destunit = destbase + xl;
          units = xr - xl;
          }
        { /* general rotation case */
          Fixed ex, ey;
          integer sData;
          integer srcPix, shift;
	  unsigned char *sample0, *sample1, *sample2, *sample3;
	  Fixed dx, dy, xStep, yStep;
	  boolean needData;
          sample0 = source->samples[0];
          sample1 = source->samples[1];
          sample2 = source->samples[2];
          sample3 = source->samples[3];
	  needData = true;
	  if (sv.x > 0) {
            dx = sv.x; xStep = ONE; ex = ONE - (fxdSrcX & LowMask); }
          else {
            dx = -sv.x; xStep = -ONE; ex = (fxdSrcX & LowMask) + 1; }
	  if (sv.y > 0) {
            dy = sv.y; yStep = ONE; ey = ONE - (fxdSrcY & LowMask); }
          else {
            dy = -sv.y; yStep = -ONE; ey = (fxdSrcY & LowMask) + 1; }
          while (units--) { /* units loop */
	    if (needData) {
	      unsigned char *srcLoc;
	      integer x, y, loc;
	      needData = false;
	      x = FTrunc(fxdSrcX) - image->info.sourceorigin.x;
	      y = FTrunc(fxdSrcY) - image->info.sourceorigin.y;
	      loc = y * wbytes;
	      switch (sourceType) {
		case s11: {
		  srcLoc = sample0 + loc + (x >> 3); 
		  data3 = (*srcLoc & (0x80 >> (x & 7))) ? 0 : 255;
		  data1 = data2 = data0 = 0;
		  break;
		  }
		case s21: {
		  x <<= 1;
		  srcLoc = sample0 + loc + (x >> 3);
		  data3 = twoBitSample[((*srcLoc & (0xC0 >> (x & 7))) >> (6 - (x & 7)))];
		  data1 = data2 = data0 = 0;
		  break;
		  }
		case s41: {
		  x <<= 2;
		  srcLoc = sample0 + loc + (x >> 3);
		  data3 = fourBitSample[(x & 4) ? (*srcLoc & 0x0F) : ((*srcLoc & 0xF0) >> 4)];
		  data1 = data2 = data0 = 0;
		  break;
		  }
		case s81: {
		  srcLoc = sample0 + loc + x;
		  data3 = 255 - *srcLoc;
		  data1 = data2 = data0 = 0;
		  break;
		  }
		case s13I: {
		  x *= 3;
		  srcLoc = sample0 + loc + (x >> 3);
		  srcPix = 0x80 >> (x & 7);
		  sData = *srcLoc;
		  data0 = (sData & srcPix) ? 0 : 255;
		  srcPix >>= 1;
		  if (srcPix == 0) {
		    srcLoc++;
		    sData = *srcLoc;
		    srcPix = 0x80;
		    }
		  data1 = (sData & srcPix) ? 0 : 255;
		  srcPix >>= 1;
		  if (srcPix == 0) {
		    srcLoc++;
		    sData = *srcLoc;
		     srcPix = 0x80;
		    }
		  data2 = (sData & srcPix) ? 0 : 255;
		  black = (data0 < data1) ? data0 : data1;
		  if (black > data2) black = data2;
		  removal = tfrUCR ? tfrUCR[black] : black;
		  data0 -= removal; if (data0 > 255) data0 = 255; else if (data0 < 0) data0 = 0;
		  data1 -= removal; if (data1 > 255) data1 = 255; else if (data1 < 0) data1 = 0;
		  data2 -= removal; if (data2 > 255) data2 = 255; else if (data2 < 0) data2 = 0;
		  data3 = tfrBG ? tfrBG[black] : black;
		  break;
		  break;
		  }
		case s23I: {
		  x *= 3;
		  x <<= 1;
		  srcLoc = sample0 + loc + (x >> 3);
		  srcPix = 0xC0 >> (x & 7); shift = 6 - (x & 7);
		  sData = *srcLoc;
		  data0 = twoBitSample[(sData & srcPix) >> shift];
		  srcPix >>= 2; shift -= 2;
		  if (srcPix == 0) {
		    srcLoc++;
		    sData = *srcLoc;
		    srcPix = 0xC0; shift = 6;
		    }
		  data1 = twoBitSample[(sData & srcPix) >> shift];
		  srcPix >>= 2; shift -= 2;
		  if (srcPix == 0) {
		    srcLoc++;
		    sData = *srcLoc;
		    srcPix = 0xC0; shift = 6;
		    }
		  data2 = twoBitSample[(sData & srcPix) >> shift];
		  black = (data0 < data1) ? data0 : data1;
		  if (black > data2) black = data2;
		  removal = tfrUCR ? tfrUCR[black] : black;
		  data0 -= removal; if (data0 > 255) data0 = 255; else if (data0 < 0) data0 = 0;
		  data1 -= removal; if (data1 > 255) data1 = 255; else if (data1 < 0) data1 = 0;
		  data2 -= removal; if (data2 > 255) data2 = 255; else if (data2 < 0) data2 = 0;
		  data3 = tfrBG ? tfrBG[black] : black;
		  break;
		  }
		case s43I: {
		  x *= 3;
		  srcLoc = sample0 + loc + (x >> 1);
		  if (x & 1) {
		    data0 = fourBitSample[*srcLoc++ & 0xf];
		    sData = *srcLoc;
		    data1 = fourBitSample[sData >> 4];
		    data2 = fourBitSample[sData & 0xf];
		    }
		  else {
		    sData = *srcLoc++;
		    data0 = fourBitSample[sData >> 4];
		    data1 = fourBitSample[sData & 0xf];
		    data2 = fourBitSample[*srcLoc >> 4];
		    }
		  black = (data0 < data1) ? data0 : data1;
		  if (black > data2) black = data2;
		  removal = tfrUCR ? tfrUCR[black] : black;
		  data0 -= removal; if (data0 > 255) data0 = 255; else if (data0 < 0) data0 = 0;
		  data1 -= removal; if (data1 > 255) data1 = 255; else if (data1 < 0) data1 = 0;
		  data2 -= removal; if (data2 > 255) data2 = 255; else if (data2 < 0) data2 = 0;
		  data3 = tfrBG ? tfrBG[black] : black;
		  break;
		  }
		case s83I: {
		  srcLoc = sample0 + loc + x*3;
		  data0 = 255 - *srcLoc++;
		  data1 = 255 - *srcLoc++;
		  data2 = 255 - *srcLoc;
		  black = (data0 < data1) ? data0 : data1;
		  if (black > data2) black = data2;
		  removal = tfrUCR ? tfrUCR[black] : black;
		  data0 -= removal; if (data0 > 255) data0 = 255; else if (data0 < 0) data0 = 0;
		  data1 -= removal; if (data1 > 255) data1 = 255; else if (data1 < 0) data1 = 0;
		  data2 -= removal; if (data2 > 255) data2 = 255; else if (data2 < 0) data2 = 0;
		  data3 = tfrBG ? tfrBG[black] : black;
		  break;
		  }
		case s14I: {
		  srcLoc = sample0 + loc + (x >> 1);
		  sData = *srcLoc;
		  if (x & 1) {
		    data0 = (8 & sData) ? 1 : 0;
		    data1 = (4 & sData) ? 1 : 0;
		    data2 = (2 & sData) ? 1 : 0;
		    data3 = (1 & sData) ? 1 : 0;
		    }
		  else {
		    data0 = (0x80 & sData) ? 1 : 0;
		    data1 = (0x40 & sData) ? 1 : 0;
		    data2 = (0x20 & sData) ? 1 : 0;
		    data3 = (0x10 & sData) ? 1 : 0;
		    }
		  break;
		  }
		case s24I: {
		  srcLoc = sample0 + loc + x;
		  sData = *srcLoc;
		  data0 = sData & 0xC0;
		  data1 = sData & 0x30;
		  data2 = sData & 0x0C;
		  data3 = sData & 0x03;
		  break;
		  }
		case s44I: {
		  srcLoc = sample0 + loc + (x << 1);
		  sData = *srcLoc++;
		  data0 = sData & 0xf0;
		  data1 = sData & 0x0f;
		  sData = *srcLoc;
		  data2 = sData & 0xf0;
		  data3 = sData & 0x0f;
		  break;
		  }
		case s84I: {
		  srcLoc = sample0 + loc + (x << 2);
		  data0 = *srcLoc++;
		  data1 = *srcLoc++;
		  data2 = *srcLoc++;
		  data3 = *srcLoc;
		  break;
		  }
		case s13N: {
		  loc += (x >> 3);
		  srcPix = 0x80 >> (x & 7);
		  data0 = (*(sample0 + loc) & srcPix) ? 0 : 255;
		  data1 = (*(sample1 + loc) & srcPix) ? 0 : 255;
		  data2 = (*(sample2 + loc) & srcPix) ? 0 : 255;
		  black = (data0 < data1) ? data0 : data1;
		  if (black > data2) black = data2;
		  removal = tfrUCR ? tfrUCR[black] : black;
		  data0 -= removal; if (data0 > 255) data0 = 255; else if (data0 < 0) data0 = 0;
		  data1 -= removal; if (data1 > 255) data1 = 255; else if (data1 < 0) data1 = 0;
		  data2 -= removal; if (data2 > 255) data2 = 255; else if (data2 < 0) data2 = 0;
		  data3 = tfrBG ? tfrBG[black] : black;
		  break;
		  break;
		  }
		case s23N: {
		  x <<= 1;
		  loc += (x >> 3);
		  srcPix = 6 - (x & 7);
		  data0 = twoBitSample[(*(sample0+loc) >> srcPix) & 0x03];
		  data1 = twoBitSample[(*(sample1+loc) >> srcPix) & 0x03];
		  data2 = twoBitSample[(*(sample2+loc) >> srcPix) & 0x03];
		  black = (data0 < data1) ? data0 : data1;
		  if (black > data2) black = data2;
		  removal = tfrUCR ? tfrUCR[black] : black;
		  data0 -= removal; if (data0 > 255) data0 = 255; else if (data0 < 0) data0 = 0;
		  data1 -= removal; if (data1 > 255) data1 = 255; else if (data1 < 0) data1 = 0;
		  data2 -= removal; if (data2 > 255) data2 = 255; else if (data2 < 0) data2 = 0;
		  data3 = tfrBG ? tfrBG[black] : black;
		  break;
		  }
		case s43N: {
		  loc += x >> 1;
		  if (x & 1) {
		    data0 = fourBitSample[*(sample0 + loc) & 0x0f];
		    data1 = fourBitSample[*(sample1 + loc) & 0x0f];
		    data2 = fourBitSample[*(sample2 + loc) & 0x0f];
		    }
		  else {
		    data0 = fourBitSample[*(sample0 + loc) >> 4];
		    data1 = fourBitSample[*(sample1 + loc) >> 4];
		    data2 = fourBitSample[*(sample2 + loc) >> 4];
		    }
		  black = (data0 < data1) ? data0 : data1;
		  if (black > data2) black = data2;
		  removal = tfrUCR ? tfrUCR[black] : black;
		  data0 -= removal; if (data0 > 255) data0 = 255; else if (data0 < 0) data0 = 0;
		  data1 -= removal; if (data1 > 255) data1 = 255; else if (data1 < 0) data1 = 0;
		  data2 -= removal; if (data2 > 255) data2 = 255; else if (data2 < 0) data2 = 0;
		  data3 = tfrBG ? tfrBG[black] : black;
		  break;
		  }
		case s83N: {
		  loc += x;
		  data0 = 255 - *(sample0 + loc);
		  data1 = 255 - *(sample1 + loc);
		  data2 = 255 - *(sample2 + loc);
		  black = (data0 < data1) ? data0 : data1;
		  if (black > data2) black = data2;
		  removal = tfrUCR ? tfrUCR[black] : black;
		  data0 -= removal; if (data0 > 255) data0 = 255; else if (data0 < 0) data0 = 0;
		  data1 -= removal; if (data1 > 255) data1 = 255; else if (data1 < 0) data1 = 0;
		  data2 -= removal; if (data2 > 255) data2 = 255; else if (data2 < 0) data2 = 0;
		  data3 = tfrBG ? tfrBG[black] : black;
		  break;
		  }
		case s14N: {
		  loc += (x >> 3);
		  srcPix = 0x80 >> (x & 7);
		  data0 = (*(sample0 + loc) & srcPix) ? 1 : 0;
		  data1 = (*(sample1 + loc) & srcPix) ? 1 : 0;
		  data2 = (*(sample2 + loc) & srcPix) ? 1 : 0;
		  data3 = (*(sample3 + loc) & srcPix) ? 1 : 0;
		  break;
		  }
		case s24N: {
		  x <<= 1;
		  loc += (x >> 3);
		  srcPix = 0xC0 >> (x & 7);
		  data0 = *(sample0+loc) & srcPix;
		  data1 = *(sample1+loc) & srcPix;
		  data2 = *(sample2+loc) & srcPix;
		  data3 = *(sample3+loc) & srcPix;
		  break;
		  }
		case s44N: {
		  loc += x >> 1;
		  if (x & 1) {
		    data0 = *(sample0 + loc) & 0x0f;
		    data1 = *(sample1 + loc) & 0x0f;
		    data2 = *(sample2 + loc) & 0x0f;
		    data3 = *(sample3 + loc) & 0x0f;
		    }
		  else {
		    data0 = *(sample0 + loc) & 0xf0;
		    data1 = *(sample1 + loc) & 0xf0;
		    data2 = *(sample2 + loc) & 0xf0;
		    data3 = *(sample3 + loc) & 0xf0;
		    }
		  break;
		  }
		case s84N: {
		  loc += x;
		  data0 = *(sample0 + loc);
		  data1 = *(sample1 + loc);
		  data2 = *(sample2 + loc);
		  data3 = *(sample3 + loc);
		  break;
		  }
		}
	      data0 = cyanV[data0] + magentaV[data1] + yellowV[data2] + blackV[data3];
	      }
	    *destunit++ = data0;
	    ey -= dy;
	    if (ey <= 0) {
	      do fxdSrcY += yStep; while ((ey += ONE) <= 0);
	      needData = true;
	      }
	    ex -= dx;
	    if (ex <= 0) {
	      do fxdSrcX += xStep; while ((ex += ONE) <= 0);
	      needData = true;
	      }
	    }
	  }
        } /* end of pairs loop */
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
  } /* end Im116 */


