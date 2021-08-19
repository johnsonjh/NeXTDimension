/* PostScript generic image module

Copyright (c) 1983, '84, '85, '86, '87, '88, '89 Adobe Systems Incorporated.
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
Paul Rovner: Wednesday, January 27, 1988 4:28:47 PM
Jim Sandman: Wed May 31 17:10:36 1989

End Edit History.
*/

/* 2, 4, 8 bit source; 1, 2, 4, 8, 16 bit dest. */

#define SMASK scanmask
#define SSHIFT scanshift
#define LOG2BPP log2BPP
#define BPP bitsPerPixel
#define PIXDELTA pixelDelta

#define PROCNAME ImS1XD1X

#define DECLAREVARS \
  integer scanshift, scanmask, log2BPP, bitsPerPixel, pixelDelta;
  
#define SETUPPARAMS(args) \
  switch (bitsPerPixel = args->bitsPerPixel) { \
    case 1: {log2BPP = 0; break; } \
    case 2: {log2BPP = 1; break; } \
    case 4: {log2BPP = 2; break; } \
    case 8: {log2BPP = 3; break; } \
    case 16: {log2BPP = 4; break; } \
    case 32: {log2BPP = 5; break; } \
    default: RAISE(ecLimitCheck, (char *)NIL); \
    } \
  scanmask = SCANMASK >> log2BPP; \
  scanshift = SCANSHIFT - log2BPP; \
  pixelDelta = args->gray.delta; 



#include "ims1xd1xmain.c"

  
