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
Ivor Durham: Wed Mar 23 13:11:32 1988
Bill Paxton: Tue Mar 29 09:25:41 1988
Paul Rovner: Fri Dec 29 11:08:10 1989
Jim Sandman: Wed Aug  9 14:48:11 1989
Joe Pasqua: Tue Feb 28 10:59:18 1989
Steve Schiller: Mon Dec 11 13:49:51 1989
End Edit History.
*/

#include PACKAGE_SPECS
#include PUBLICTYPES
#include DEVICETYPES
#include DEVIMAGE
#include DEVPATTERN
#include EXCEPT
#include PSLIB

#include "imagepriv.h"
#include "patternpriv.h"

#define BPP bitsPerPixel

private integer twoBitSample[4] = {0, 85, 170, 255};

typedef enum {
  s11, s21, s41, s81, s13I, s23I, s43I, s83I, s14I, s24I, s44I, s84I,
  s13N, s23N, s43N, s83N, s14N, s24N, s44N, s84N, sBad
  } ImSourceType;
  
public procedure ImSXXD3X(items, t, run, args)
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
  DevScreen *scrRed = info->halftone->red;
  unsigned char *scrEltRed;
  unsigned char *sScrRowRed;
  unsigned char *endScrRowRed;
  integer scrRowRed;
  private PSCANTYPE redV, redT;
  integer pixDeltaRed;
  DevScreen *scrGreen = info->halftone->green;
  unsigned char *scrEltGreen;
  unsigned char *sScrRowGreen;
  unsigned char *endScrRowGreen;
  integer scrRowGreen;
  private PSCANTYPE greenV, greenT;
  integer pixDeltaGreen;
  DevScreen *scrBlue = info->halftone->blue;
  unsigned char *scrEltBlue;
  unsigned char *sScrRowBlue;
  unsigned char *endScrRowBlue;
  integer scrRowBlue;
  private PSCANTYPE blueV, blueT;
  integer pixDeltaBlue;
  integer y, ylast, mpwCBug, pairs, wbytes,
    xoffset, yoffset, txl, txr, xx, yy, units, scrnCol,
    pixelX, pixelXr, bitr, srcXprv, srcYprv, lastPixel;
  DevShort *buffptr;
  DevPoint grayOrigin;
  DevFixedPoint sv, svy;
  Fixed fxdSrcX, fxdSrcY, fX, fY, srcLX, srcLY, srcGX, srcGY, endX, endY;
  boolean leftSlope, rightSlope;
  PSCANTYPE destunit, destbase;
  PVoidProc sampleProc;
  SCANTYPE maskl, maskr;
  integer scanmask;
  integer scanshift;
  integer bitsPerPixel, log2BPP;
  ImSourceType sourceType = sBad;

  switch (bitsPerPixel = args->bitsPerPixel) {
    case 1: {log2BPP = 0; break; }
    case 2: {log2BPP = 1; break; }
    case 4: {log2BPP = 2; break; }
    case 8: {log2BPP = 3; break; }
    case 16: {log2BPP = 4; break; }
    case 32: {log2BPP = 5; break; }
    default: RAISE(ecLimitCheck, (char *)NIL);
    }
  if (!ValidateTA(scrRed) || !ValidateTA(scrGreen) || !ValidateTA(scrBlue))
    CantHappen();
  redV = GetRedPixBuffers();
  redT = redThresholds;
  greenV = GetGreenPixBuffers();
  greenT = greenThresholds;
  blueV = GetBluePixBuffers();
  blueT = blueThresholds;
  
  scanmask = SCANMASK >> log2BPP;
  scanshift = SCANSHIFT - log2BPP;
  
  {
  PCard8 tfrR, tfrG, tfrB;
  PVoidProc setValuesProc;
  integer i;
  if (image->transfer == NULL)
    tfrR = tfrG = tfrB = NULL;
  else {
    tfrR = image->transfer->red;
    tfrG = image->transfer->green;
    tfrB = image->transfer->blue;
    };
  if (source->colorSpace == DEVCMYK_COLOR_SPACE)
    setValuesProc = Set8BitValues;
  else
    switch (source->bitspersample) {
      case 1:
	setValuesProc = Set1BitValues;
	break;
      case 2:
	setValuesProc = Set2BitValues;
	break;
      case 4:
	setValuesProc = Set4BitValues;
	break;
      case 8:
	setValuesProc = Set8BitValues;
	break;
      }
  (*setValuesProc)(
    args->red, tfrR, redV, redT, &source->decode[IMG_RED_SAMPLES]);
  (*setValuesProc)(
    args->green, tfrG, greenV, greenT, &source->decode[IMG_GREEN_SAMPLES]);
  (*setValuesProc)(
    args->blue, tfrB, blueV, blueT, &source->decode[IMG_BLUE_SAMPLES]);
  if (args->firstColor != 0)
    for (i = 0; i < 256; i++) redV[i] += args->firstColor;
  }
  
  pixDeltaRed = args->red.delta;
  pixDeltaGreen = args->green.delta;
  pixDeltaBlue = args->blue.delta;
  wbytes = source->wbytes;

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
    grayOrigin.x = info->offset.x + info->screenphase.x;
    grayOrigin.y = info->offset.y + info->screenphase.y;
    scrRowRed = y - grayOrigin.y;
    mpwCBug = (scrRowRed / scrRed->height);
    mpwCBug = mpwCBug * scrRed->height;
    scrRowRed = scrRowRed - mpwCBug;
    if (scrRowRed < 0) scrRowRed += scrRed->height;
    sScrRowRed = scrRed->thresholds + scrRowRed * scrRed->width;
    
    scrRowGreen = y - grayOrigin.y;
    mpwCBug = (scrRowGreen / scrGreen->height);
    mpwCBug = mpwCBug * scrGreen->height;
    scrRowGreen = scrRowGreen - mpwCBug;
    if (scrRowGreen < 0) scrRowGreen += scrGreen->height;
    sScrRowGreen = scrGreen->thresholds + scrRowGreen * scrGreen->width;
    
    scrRowBlue = y - grayOrigin.y;
    mpwCBug = (scrRowBlue / scrBlue->height);
    mpwCBug = mpwCBug * scrBlue->height;
    scrRowBlue = scrRowBlue - mpwCBug;
    if (scrRowBlue < 0) scrRowBlue += scrBlue->height;
    sScrRowBlue = scrBlue->thresholds + scrRowBlue * scrBlue->width;
    
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
          pixelX = (xl & scanmask) << log2BPP;
          pixelXr = (xr & scanmask) << log2BPP;
          scrnCol = xl - grayOrigin.x;
          { register int unitl;
            unitl = xl >> scanshift;
            destunit = destbase + unitl;
            units = (xr >> scanshift) - unitl;
            }
          }
        maskl = leftBitArray[pixelX];
        if (pixelXr == 0) {
	  units--; pixelXr = SCANUNIT; maskr = -1; bitr = SCANUNIT;
	  }
        else {
	  bitr = pixelXr; maskr = rightBitArray[bitr];
	  };
        endScrRowRed = sScrRowRed + scrRed->width;
        endScrRowGreen = sScrRowGreen + scrGreen->width;
        endScrRowBlue = sScrRowBlue + scrBlue->width;
        { register int m, k;
          m = scrRed->width; k = scrnCol; /* k may be < 0 so cannot use % */
          k = k - (k/m)*m; if (k < 0) k += m;
          scrEltRed = sScrRowRed + k;
          m = scrGreen->width; k = scrnCol; /* k may be < 0 so cannot use % */
          k = k - (k/m)*m; if (k < 0) k += m;
          scrEltGreen = sScrRowGreen + k;
          m = scrBlue->width; k = scrnCol; /* k may be < 0 so cannot use % */
          k = k - (k/m)*m; if (k < 0) k += m;
          scrEltBlue = sScrRowBlue + k;
          }
	lastPixel = (units > 0) ? SCANUNIT : pixelXr;
        { /* general rotation case */
          register int bb;
          SCANTYPE temp;
          Fixed ex, ey;
          integer sData;
          integer data0, data1, data2, data3;
          integer red, green, blue;
          integer r, g, b;
          integer srcPix;
	  unsigned char *sample0, *sample1, *sample2, *sample3;
	  Fixed dx, dy, xStep, yStep;
	  boolean needData;
          temp = *destunit & ~maskl;
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
          while (true) { /* units loop */
            bb = lastPixel - pixelX - bitsPerPixel;
	    do { /* pixelX loop */
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
		    data0 = (*srcLoc & (0x80 >> (x & 7))) ? 255 : 0;
		    data1 = data2 = data0;
		    break;
		    }
		  case s21: {
		    x <<= 1;
		    srcLoc = sample0 + loc + (x >> 3);
		    data0 = (*srcLoc & (0xC0 >> (x & 7)));
		    data1 = data2 = data0;
		    break;
		    }
		  case s41: {
		    x <<= 2;
		    srcLoc = sample0 + loc + (x >> 3);
		    data0 = (*srcLoc & (0xF0 >> (x & 7)));
		    data1 = data2 = data0;
		    break;
		    }
		  case s81: {
		    srcLoc = sample0 + loc + x;
		    data0 = *srcLoc;
		    data1 = data2 = data0;
		    break;
		    }
		  case s13I: {
		    x *= 3;
		    srcLoc = sample0 + loc + (x >> 3);
		    srcPix = 0x80 >> (x & 7);
		    sData = *srcLoc;
		    data0 = (sData & srcPix) ? 255 : 0;
		    srcPix >>= 1;
		    if (srcPix == 0) {
		      srcLoc++;
		      sData = *srcLoc;
		      srcPix = 0x80;
		      }
		    data1 = (sData & srcPix) ? 255 : 0;
		    srcPix >>= 1;
		    if (srcPix == 0) {
		      srcLoc++;
		      sData = *srcLoc;
 		      srcPix = 0x80;
		      }
		    data2 = (sData & srcPix) ? 255 : 0;
		    break;
		    }
		  case s23I: {
		    x *= 3;
		    x <<= 1;
		    srcLoc = sample0 + loc + (x >> 3);
		    srcPix = 0xC0 >> (x & 7);
		    sData = *srcLoc;
		    data0 = sData & srcPix;
		    srcPix >> 2;
		    if (srcPix == 0) {
		      srcLoc++;
		      sData = *srcLoc;
		      srcPix = 0xC0;
		      }
		    data1 = sData & srcPix;
		    srcPix >> 2;
		    if (srcPix == 0) {
		      srcLoc++;
		      sData = *srcLoc;
		      srcPix = 0xC0;
		      }
		    data2 = sData & srcPix;
		    break;
		    }
		  case s43I: {
		    x *= 3;
		    srcLoc = sample0 + loc + (x >> 1);
		    if (x & 1) {
		      data0 = *srcLoc++ & 0xf;
		      sData = *srcLoc;
		      data1 = sData >> 4;
		      data2 = sData & 0xf;
		      }
		    else {
		      sData = *srcLoc++;
		      data0 = sData >> 4;
		      data1 = sData & 0xf;
		      data2 = *srcLoc >> 4;
		      }
		    break;
		    }
		  case s83I: {
		    srcLoc = sample0 + loc + x*3;
		    data0 = *srcLoc++;
		    data1 = *srcLoc++;
		    data2 = *srcLoc;
		    break;
		    }
		  case s14I: {
		    srcLoc = sample0 + loc + (x >> 1);
		    sData = *srcLoc;
		    if (x & 1) {
		      data0 = (8 & sData) ? 0 : 255;
		      data1 = (4 & sData) ? 0 : 255;
		      data2 = (2 & sData) ? 0 : 255;
		      data3 = (1 & sData) ? 255 : 0;
		      }
		    else {
		      data0 = (0x80 & sData) ? 0 : 255;
		      data1 = (0x40 & sData) ? 0 : 255;
		      data2 = (0x20 & sData) ? 0 : 255;
		      data3 = (0x10 & sData) ? 255 : 0;
		      }
		    data0 -= data3;
		    data1 -= data3;
		    data2 -= data3;
		    if (data0 < 0) data0 = 0;
		    if (data1 < 0) data1 = 0;
		    if (data2 < 0) data2 = 0;
		    break;
		    }
		  case s24I: {
		    srcLoc = sample0 + loc + x;
		    sData = *srcLoc;
		    data0 = twoBitSample[sData >> 6];
		    data1 = twoBitSample[3 & (sData >> 4)];
		    data2 = twoBitSample[3 & (sData >> 2)];
		    data3 = twoBitSample[3 & sData];
		    data0 = 255 - data0 - data3;
		    data1 = 255 - data1 - data3;
		    data2 = 255 - data2 - data3;
		    if (data0 < 0) data0 = 0;
		    if (data1 < 0) data1 = 0;
		    if (data2 < 0) data2 = 0;
		    break;
		    }
		  case s44I: {
		    srcLoc = sample0 + loc + (x << 1);
		    sData = *srcLoc++;
		    data0 = sData >> 4;
		    data1 = sData & 0xf;
		    sData = *srcLoc;
		    data2 = sData >> 4;
		    data3 = sData & 0xf;
		    data0 |= data0 << 4;
		    data1 |= data1 << 4;
		    data2 |= data2 << 4;
		    data3 |= data3 << 4;
		    data0 = 255 - data0 - data3;
		    data1 = 255 - data1 - data3;
		    data2 = 255 - data2 - data3;
		    if (data0 < 0) data0 = 0;
		    if (data1 < 0) data1 = 0;
		    if (data2 < 0) data2 = 0;
		    break;
		    }
		  case s84I: {
		    srcLoc = sample0 + loc + (x << 2);
		    data0 = *srcLoc++;
		    data1 = *srcLoc++;
		    data2 = *srcLoc++;
		    data3 = *srcLoc;
		    data0 = 255 - data0 - data3;
		    data1 = 255 - data1 - data3;
		    data2 = 255 - data2 - data3;
		    if (data0 < 0) data0 = 0;
		    if (data1 < 0) data1 = 0;
		    if (data2 < 0) data2 = 0;
		    break;
		    }
		  case s13N: {
		    loc += (x >> 3);
		    srcPix = 0x80 >> (x & 7);
		    data0 = (*(sample0 + loc) & srcPix) ? 255 : 0;
		    data1 = (*(sample1 + loc) & srcPix) ? 255 : 0;
		    data2 = (*(sample2 + loc) & srcPix) ? 255 : 0;
		    break;
		    }
		  case s23N: {
		    x <<= 1;
		    loc += (x >> 3);
		    srcPix = 0xC0 >> (x & 7);
		    data0 = *(sample0+loc) & srcPix;
		    data1 = *(sample1+loc) & srcPix;
		    data2 = *(sample2+loc) & srcPix;
		    break;
		    }
		  case s43N: {
		    loc += x >> 1;
		    if (x & 1) {
		      data0 = *(sample0 + loc) & 0xf;
		      data1 = *(sample1 + loc) & 0xf;
		      data2 = *(sample2 + loc) & 0xf;
		      }
		    else {
		      data0 = *(sample0 + loc) >> 4;
		      data1 = *(sample1 + loc) >> 4;
		      data2 = *(sample2 + loc) >> 4;
		      }
		    break;
		    }
		  case s83N: {
		    loc += x;
		    data0 = *(sample0 + loc);
		    data1 = *(sample1 + loc);
		    data2 = *(sample2 + loc);
		    break;
		    }
		  case s14N: {
		    loc += (x >> 3);
		    srcPix = 0x80 >> (x & 7);
		    data0 = (*(sample0 + loc) & srcPix) ? 0 : 255;
		    data1 = (*(sample1 + loc) & srcPix) ? 0 : 255;
		    data2 = (*(sample2 + loc) & srcPix) ? 0 : 255;
		    data3 = (*(sample3 + loc) & srcPix) ? 255 : 0;
		    data0 -= data3;
		    data1 -= data3;
		    data2 -= data3;
		    if (data0 < 0) data0 = 0;
		    if (data1 < 0) data1 = 0;
		    if (data2 < 0) data2 = 0;
		    break;
		    }
		  case s24N: {
		    x <<= 1;
		    loc += (x >> 3);
		    srcPix = 6 - (x & 7);
		    data0 = 
		      twoBitSample[(*(sample0+loc) >> srcPix) & 3];
		    data1 =
		      twoBitSample[(*(sample1+loc) >> srcPix) & 3];
		    data2 =
		      twoBitSample[(*(sample2+loc) >> srcPix) & 3];
		    data3 =
		      twoBitSample[(*(sample3+loc) >> srcPix) & 3];
		    data0 = 255 - data0 - data3;
		    data1 = 255 - data1 - data3;
		    data2 = 255 - data2 - data3;
		    if (data0 < 0) data0 = 0;
		    if (data1 < 0) data1 = 0;
		    if (data2 < 0) data2 = 0;
		    break;
		    }
		  case s44N: {
		    loc += x >> 1;
		    if (x & 1) {
		      data0 = *(sample0 + loc) & 0xf;
		      data1 = *(sample1 + loc) & 0xf;
		      data2 = *(sample2 + loc) & 0xf;
		      data3 = *(sample3 + loc) & 0xf;
		      }
		    else {
		      data0 = *(sample0 + loc) >> 4;
		      data1 = *(sample1 + loc) >> 4;
		      data2 = *(sample2 + loc) >> 4;
		      data3 = *(sample3 + loc) >> 4;
		      }
		    data0 |= data0 << 4;
		    data1 |= data1 << 4;
		    data2 |= data2 << 4;
		    data3 |= data3 << 4;
		    data0 = 255 - data0 - data3;
		    data1 = 255 - data1 - data3;
		    data2 = 255 - data2 - data3;
		    if (data0 < 0) data0 = 0;
		    if (data1 < 0) data1 = 0;
		    if (data2 < 0) data2 = 0;
		    break;
		    }
		  case s84N: {
		    loc += x;
		    data0 = *(sample0 + loc);
		    data1 = *(sample1 + loc);
		    data2 = *(sample2 + loc);
		    data3 = *(sample3 + loc);
		    data0 = 255 - data0 - data3;
		    data1 = 255 - data1 - data3;
		    data2 = 255 - data2 - data3;
		    if (data0 < 0) data0 = 0;
		    if (data1 < 0) data1 = 0;
		    if (data2 < 0) data2 = 0;
		    break;
		    }
		  }
		r = redV[data0];
		data0 = redT[data0];
		g = greenV[data1];
		data1 = greenT[data1];
		b = blueV[data2];
		data2 = blueT[data2];
	        }
	      red = (data0 >= *scrEltRed++) ? r - pixDeltaRed : r;
	      green = (data1 >= *scrEltGreen++) ? g - pixDeltaGreen : g;
	      blue = (data2 >= *scrEltBlue++) ? b - pixDeltaBlue : b;
	      temp |= SHIFTPIXEL(red+green+blue,bb);
	      if (scrEltRed >= endScrRowRed)
	        scrEltRed -= scrRed->width;
	      if (scrEltGreen >= endScrRowGreen)
	        scrEltGreen -= scrGreen->width;
	      if (scrEltBlue >= endScrRowBlue)
	        scrEltBlue -= scrBlue->width;
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
	      } while ((bb -= bitsPerPixel) >= 0); /* end of pixel loop */
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
      if (++scrRowRed == scrRed->height) {
        scrRowRed = 0; sScrRowRed = scrRed->thresholds; }
      else sScrRowRed += scrRed->width;
      if (++scrRowGreen == scrGreen->height) {
        scrRowGreen = 0; sScrRowGreen = scrGreen->thresholds; }
      else sScrRowGreen += scrGreen->width;
      if (++scrRowBlue == scrBlue->height) {
        scrRowBlue = 0; sScrRowBlue = scrBlue->thresholds; }
      else sScrRowBlue += scrBlue->width;
      }
    if (t != NULL) t++;
    }
  (*args->procs->End)(args->data);
  } /* end Image4 */


