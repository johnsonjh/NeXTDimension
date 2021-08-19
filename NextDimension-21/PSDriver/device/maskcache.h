/*
  maskcache.h

Copyright (c) 1988 Adobe Systems Incorporated.
All rights reserved.

NOTICE:  All information contained herein is the property of Adobe Systems
Incorporated.  Many of the intellectual and technical concepts contained
herein are proprietary to Adobe, are protected as trade secrets, and are made
available only to Adobe licensees for their internal use.  Any reproduction
or dissemination of this software is strictly forbidden unless prior written
permission is obtained from Adobe.

PostScript is a registered trademark of Adobe Systems Incorporated.
Display PostScript is a trademark of Adobe Systems Incorporated.

Edit History:
Jim Sandman: Wed Mar 29 14:47:36 1989
Paul Rovner: Wed Aug  9 13:30:23 1989
End Edit History.
*/

#ifndef	MASKCACHE_H
#define	MASKCACHE_H

#include PUBLICTYPES
#include ENVIRONMENT
#include DEVICE


#define BMFreeTag 0
#define BMUsedTag 1
#define BMPinnedTag 2

typedef struct {		/* variable length items in the VM level of */
  BitField	tag:2;	/*  */
  BitField	maskIndex:14;	/* for BMItem */
  Card16	length;		/* number of words (4-bytes) */
}  BMHeader,
  *PBMHeader;

typedef struct _t_BMItem {
  BMHeader header;
  struct _t_BMItem *flink, *blink;
} BMItem, *PBMItem;

#define BMNull (PBMItem)(NULL)


/* allows the BM allocator to deal in byte adresses */

#define Header(o) ((BMHeader)(&((o)->header)))
#define Data(o) ((PCard8)o + sizeof(BMHeader))

extern PMask MCGetMask();
extern char *MCGetTempBytes(/* integer nBytes; */);
extern procedure MCGetCacheBytes(/* PMask mask; integer nBytes; */);
extern procedure MCFreeBytes(/* char *bytes; */);
extern procedure MCFreeMask(/* PMask mask; */);

extern procedure DevAddMaskRef(/* PMask mask; */);
extern procedure DevRemMaskRef(/* PMask mask; */);

#endif	MASKCACHE_H
