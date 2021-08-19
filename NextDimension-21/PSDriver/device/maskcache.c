/*
  maskcache.c

Copyright (c) 1984, '85, '86, '87, '88 Adobe Systems Incorporated.
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
Ed Taft: Fri Jul 29 09:36:07 1988
Ivor Durham: Sun Aug 14 14:00:13 1988
Jim Sandman: Thu Nov  2 09:41:52 1989
Perry Caro: Wed Nov  9 14:16:42 1988
Joe Pasqua: Tue Feb 28 13:43:07 1989
Bill Bilodeau: Fri Jan 20 16:45:41 PST 1989
Paul Rovner: Wed Nov 22 14:54:38 1989
Peter Graffagnino 4/27/90 applied adobe font cache fix
End Edit History.
*/

#include PACKAGE_SPECS
#include PUBLICTYPES
#include ENVIRONMENT
#include EXCEPT
#include DEVICE
#include PSLIB
#include FOREGROUND
#include STREAM

#include "maskcache.h"

#define NULLMaskIndex MAXCard16

private longcardinal scratchBytes;
private boolean scratchInUse;
private integer bmUsed, bmSize, bmBytes, bmMinSize, bmMaxSize;

private PMask mskBase, mskLast, mskFree;
private PBMItem bmBase, bmFree, bmScratch;

#if STAGE==DEVELOP
public boolean mcdebug, mchange, mcCheck, nUsedMasks;
#endif STAGE==DEVELOP

#define MINBMALLOC ((2 * sizeof(BMItem)) / sizeof(BMHeader))
#define MINBMFLUSH 10000

#if STAGE==DEVELOP
public procedure CheckBM()
{
  register PBMItem o, b, n; register integer count, ucount;
  PBMItem ustart;
  PMask mask;
  integer ulength;
  if (!mcCheck) return;
  FGEnterMonitor();
  ulength = 0; ucount = 0;
  o = bmBase; count = 0;
  while (o <= bmScratch){
    Assert(o->header.length >= MINBMALLOC);
    n = (PBMItem)((PBMHeader)o + o->header.length);
    Assert(n <= (PBMItem)((PBMHeader)bmScratch+bmScratch->header.length));
    if (o->header.tag == BMFreeTag){
      if (mcdebug && mchange){
	if (ulength != 0){
	  os_eprintf("Used -- offset: %d, total-length: %d, segs: %d\n",
		ustart,ulength,ucount);
	  ulength = 0; ucount = 0;}
	os_eprintf("Free -- offset: %d, length: %d, links: (%d,%d)\n",
	     o,o->header.length,o->blink,o->flink);
	fflush(os_stdout);}}
    else{
      Assert(o->header.tag == BMUsedTag || o->header.tag == BMPinnedTag);
      if (ulength == 0) ustart = o;
      ulength += o->header.length; ucount++;
      count += o->header.length;}
    o = n;
    }
  if (mcdebug && mchange && (ulength != 0)){
	os_eprintf("Used -- offset: %d, total-length: %d, segs: %d\n",
		ustart,ulength,ucount);
	fflush(os_stderr);}
  Assert((count*sizeof(BMHeader))==bmUsed);
  o = bmFree; b = BMNull; count = 0;
  while (o != BMNull){
    Assert(o->header.tag == BMFreeTag);
    Assert(o->blink == b);
    b = o; count += o->header.length;
    o = o->flink;}
  mask = mskFree;
#if 0
  count *= sizeof(BMHeader);
  while (mask != NULL) {
    count += sizeof(MaskRec);
    mask = (PMask)mask->data;
    }
  Assert(bmUsed == (bmSize - count));
#endif
  mchange = false;
  FGExitMonitor();
}
#endif STAGE==DEVELOP

private procedure BMLink(item) register PBMItem item; {
  item->blink = BMNull;
  item->flink = bmFree;
  if (bmFree != BMNull)
    bmFree->blink = item;
  bmFree = item;
  }

private procedure BMUnlink(item) register PBMItem item; {
  register PBMItem b,f;
  b = item->blink; f = item->flink;
  if (b == BMNull){
    DebugAssert(bmFree == item);
    bmFree = f;}
  else {
    DebugAssert(bmFree != item);
    DebugAssert(b->header.tag == BMFreeTag);
    b->flink = f;}
  if (f != BMNull){
    DebugAssert(f->header.tag == BMFreeTag);
    f->blink = b;}
  }

public procedure CompactBM() {
  register PBMItem bmf;
  register PBMHeader o;
  register integer l, ol;
#if STAGE==DEVELOP
  boolean onePinned = false;
#endif STAGE==DEVELOP
  FGEnterMonitor();
#if STAGE==DEVELOP
  CheckBM();
#endif STAGE==DEVELOP
  o = &bmBase->header;
  bmFree = bmf = BMNull;
topofloop:
  while (o < &bmScratch->header){
    if (o->tag == BMFreeTag ) {
      if (bmf == NULL) {bmf = (PBMItem)o;}
      else {
	if (bmf != (PBMItem)o) {
	  bmf->header.length += o->length;
	  }
	}
      }
    else {
      Assert(o->tag == BMUsedTag || o->tag == BMPinnedTag);
      if (bmf != NULL) {
	if (o->tag == BMUsedTag) {
	  DebugAssert(
	    (((PBMHeader)&bmf->header)+bmf->header.length) == o);
	  if (o->maskIndex != NULLMaskIndex)
	    mskBase[o->maskIndex].data = Data(bmf);
	  l = bmf->header.length; ol = o->length;
	  os_bcopy(
	    (char *)(o),(char *)(bmf),(long int)(ol*sizeof(BMHeader)));
	  bmf = (PBMItem)((PCard8)bmf+ol*sizeof(BMHeader));
	  bmf->header.tag = BMFreeTag;
	  bmf->header.length = l;
	  o = &bmf->header; goto topofloop;
	  }
	else {
	  BMLink(bmf);
	  bmf = BMNull;
#if STAGE==DEVELOP
	  onePinned = true;
#endif STAGE==DEVELOP
	  }
        }
      }
    o += o->length;
    }
  if (bmf != BMNull) BMLink(bmf);
#if STAGE==DEVELOP
  else if (!onePinned) {Assert((bmFree == BMNull) ||
	     ((bmFree->flink == BMNull) &&
	      (bmFree->blink == BMNull)));}
  CheckBM();
#endif STAGE==DEVELOP
  FGExitMonitor();
}

private PBMItem BMAlloc(nBytes) integer nBytes;
{
  register PBMItem o,f; register Card32 l, ll;
  integer max;
  integer flushAmt = 25000;
  register integer n;
  FGEnterMonitor();
  DebugAssert(nBytes > 0);
  n = ((nBytes + sizeof(BMHeader) - 1) / sizeof(BMHeader)) + 1;
  if ( n < MINBMALLOC) n = MINBMALLOC;
#if STAGE==DEVELOP
  if (mcdebug) os_eprintf("ENTER alloc: %d, %d\n",n, nBytes);
  CheckBM();
#endif STAGE==DEVELOP
  while (true) {
    o = bmFree;
    while (o != BMNull) {
      Assert(o->header.tag == BMFreeTag);
      while (true) {
	if ((l = o->header.length) >= n){
	  BMUnlink(o);
	  o->header.tag = BMUsedTag;
	  if (l - n > MINBMALLOC) {
	    f = (PBMItem)(((PBMHeader)o) + n);
	    f->header.tag = BMFreeTag;
	    f->header.length = l-n;
	    BMLink(f);
	    o->header.length = n;
	    }
	  bmUsed += o->header.length * sizeof(BMHeader);
	  goto success;
	  }
	f = (PBMItem)((PBMHeader)o + l);
	if (f->header.tag != BMFreeTag)
	  break;
	BMUnlink(f);
	o->header.length += f->header.length;
	}
      o = o->flink;
      }
    if ((bmSize-bmUsed) >= n * sizeof(BMHeader)) {
      max = 0;
      CompactBM();
      for (f = bmFree; f != BMNull; f = f->flink)
	if (max < f->header.length) max = f->header.length;
      if (max >= n) continue;
      }
    if (!PSFlushMasks(os_max(n*sizeof(BMHeader) + sizeof(MaskRec), MINBMFLUSH), -1)) {
      o = BMNull;
      break;
      }
    }
success:
#if STAGE==DEVELOP
  mchange = true;
  CheckBM();
  if (mcdebug) os_eprintf("EXIT alloc: %x\n",o);
#endif STAGE==DEVELOP
  FGExitMonitor();
  return o;
  }

public procedure BMFree(o) register PBMItem o;
  {
  register integer l = o->header.length;
  PBMItem n;
  FGEnterMonitor();
#if STAGE==DEVELOP
  Assert(l >= MINBMALLOC);
  if (mcdebug) os_eprintf("ENTER free: %x\n",o);
#endif STAGE==DEVELOP
  Assert(o->header.tag == BMUsedTag || o->header.tag == BMPinnedTag);
  o->header.tag = BMFreeTag;
  BMLink(o);
  n = (PBMItem)(((PBMHeader)o)+l);
  while(n->header.tag == BMFreeTag){ /* coalesce with next block */
    BMUnlink(n);
    l += n->header.length;
    n = (PBMItem)(((PBMHeader)o)+l);
    }
  o->header.length = l;
#if STAGE==DEVELOP
  mchange = true;
  CheckBM();
  if (mcdebug) os_eprintf("EXIT free:\n");
#endif STAGE==DEVELOP
  FGExitMonitor();
  }


public char *MCGetTempBytes(nBytes) integer nBytes; {
  PBMItem item;
  FGEnterMonitor();
  if (!scratchInUse && scratchBytes >= nBytes) {
    scratchInUse = true;
    FGExitMonitor();
    return (char *)Data(bmScratch);
    }
  FGExitMonitor();
  item = BMAlloc(nBytes);
  if (item == BMNull) {
    return (char *)NULL;
    }
  else {
    item->header.tag = BMPinnedTag;
    return (char *)Data(item);
    }
  }		


public integer MCMaskIndex(mask) PMask mask; {
  return (integer)(mask - mskBase);
  }

public procedure MCGetCacheBytes(mask, nBytes) PMask mask; integer nBytes; {
  PBMItem item;
  item = BMAlloc(nBytes);
  if (item == BMNull) mask->data = NIL;
  else {
    item->header.maskIndex = MCMaskIndex(mask);
    mask->data = Data(item);
    }
  }		

public procedure MCFreeBytes(bytes) char *bytes; {
  PBMItem item = (PBMItem)(bytes - sizeof(BMHeader));
  FGEnterMonitor();
  if (item == bmScratch)
    scratchInUse = false;
  else {
    bmUsed -= item->header.length * sizeof(BMHeader);
    BMFree(item);
    }
  FGExitMonitor();
  }
  
public PMask MCGetMask() {
  PMask mask;
  FGEnterMonitor();
  while (true) {
    if (mskFree != (PMask)NULL) {
      mask = mskFree;
      mskFree = (PMask)mask->data;
      mask->data = NIL;
#if STAGE==DEVELOP
      nUsedMasks++;
#endif STAGE==DEVELOP
      FGExitMonitor();
      return mask;
      }
    if (PSFlushMasks(sizeof(MaskRec), -1) < sizeof(MaskRec)) break;
    }
  FGExitMonitor();
  return (PMask)NULL;
  }


public procedure MCFreeMask(mask) PMask mask; {
  FGEnterMonitor();
  if (mask->data != NULL)
    MCFreeBytes(mask->data);
  mask->data = (unsigned char *)mskFree;
  mask->unused = 0;
  mskFree = mask;
#if STAGE==DEVELOP
  nUsedMasks--;
#endif STAGE==DEVELOP
  FGExitMonitor();
  }
 
public integer DevFlushMask(mask, args) PMask mask; DevFlushMaskArgs *args; {
  integer ans = 0;
  unsigned c;
  if (mask == NIL) return ans;
  FGEnterMonitor();
#if DISKDL
  if (args && !mask->onDisk && (mask->data != NIL))
    PutMaskInDiskCache(mask, args);
#endif
#if 0
  c = mask->unused;
  if ((c & 0x3FFF) != 0) /* hold onto it until refcount goes to 0 */
    mask->unused |= 0x4000;
  else 
#endif
  { /* refcount is 0; can really flush it */
    integer oldUsed = bmUsed;
    DebugAssert(mask >= mskBase && mask < mskLast);
    MCFreeMask(mask);
    ans = oldUsed - bmUsed;
    }
  FGExitMonitor();
  return ans;
  }
  
#if 0
public procedure DevAddMaskRef(mask) PMask mask; {
  unsigned c;
  if (mask == NIL) return;
  c = mask->unused;
  if ((c & 0x3FFF) != 0x3FFF) /* reference count is not pinned */
    mask->unused = c + 1;
  }

public procedure DevRemMaskRef(mask) PMask mask; {
  unsigned c;
  if (mask == NIL) return;
  c = mask->unused;
  if ((c & 0x3FFF) != 0x3FFF) { /* reference count is not pinned */
    Assert((c & 0x3FFF) != 0);
    c--;
    mask->unused = c;
    if (c == 0x4000)
      DevFlushMask(mask, NIL);
    }
  }
#endif

private procedure GrowBM(size)
integer	size;
{
register PBMItem oldbase;
register PBMHeader o;

  CompactBM();
  oldbase = bmBase;
  bmBase = (PBMItem) EXPAND(bmBase, size, (integer)1);
  if(bmBase != oldbase) {		/* table was moved */
	bmScratch = (PBMItem)(((char *)bmScratch - (char *)oldbase) + (char *)bmBase);
	bmFree = (PBMItem)(((char *)bmFree - (char *)oldbase) + (char *)bmBase);
  	o = &bmBase->header;
	while(o < &bmFree->header) {		/* update mask record table */
		Assert(o->tag == BMUsedTag);
	    	mskBase[o->maskIndex].data = Data(o);
		o += o->length;
	}
  }
  o = &bmScratch->header;
  bmScratch = (PBMItem)((PBMHeader)bmBase + size/sizeof(BMHeader) 
  				- bmScratch->header.length);
  os_bcopy((char *)o, (char *)bmScratch,(long int)(o->length * sizeof(BMHeader)));
  bmBytes = size;
  bmFree->header.length = ((char *)bmScratch - (char *)bmFree)/sizeof(BMHeader);
}

private procedure ShrinkBM(size)
integer size;
{
register PBMItem oldbase;
register PBMHeader o;
integer amountToFlush, n;

  CompactBM();
  if(bmFree->header.length < (n=(bmBytes - size)/sizeof(BMHeader))) {
   	amountToFlush = (n - bmFree->header.length) * sizeof(BMHeader);
  	size = PSFlushMasks(amountToFlush, -1); /* try to flush the cache */
  	CompactBM();
  }
  o = &bmScratch->header;
  bmScratch = (PBMItem)((PBMHeader)bmBase + size/sizeof(BMHeader) 
  				- bmScratch->header.length);
  os_bcopy((char *)o, (char *)bmScratch,(long int)(o->length * sizeof(BMHeader)));
  oldbase = bmBase;
  bmBase = (PBMItem) EXPAND(bmBase, size, (integer)1);
  if(bmBase != oldbase) {		/* table was moved */
	bmScratch = (PBMItem)(((char *)bmScratch - (char *)oldbase) + (char *)bmBase);
	bmFree = (PBMItem)(((char *)bmFree - (char *)oldbase) + (char *)bmBase);
  	o = &bmBase->header;
	while(o < &bmFree->header) {		/* update mask record table */
		Assert(o->tag == BMUsedTag);
	    	mskBase[o->maskIndex].data = Data(o);
		o += o->length;
	}
  }
  bmBytes = size;
  bmFree->header.length = ((char *)bmScratch - (char *)bmFree)/sizeof(BMHeader);
}

public procedure DevMaskCacheInfo(used, size) integer *used, *size; {
  FGEnterMonitor();
  *used = bmUsed;
  *size = bmSize;
  FGExitMonitor();
  }

public procedure DevSetMaskCacheSize(size,minimum) integer size, minimum; {
  FGEnterMonitor();
  if (size > bmMaxSize) size = bmMaxSize;
  if (size < bmMinSize) size = bmMinSize;
  if (size < minimum) size = minimum; /* as determined by CacheThreshold */
  if(size > bmBytes)
  	GrowBM(size);
  else
  	ShrinkBM(size);
  bmSize = size;
  FGExitMonitor();
  }

public procedure InitMaskCache(nMasks, nBytes, minSize, maxSize, sBytes)
  integer nMasks, nBytes, minSize, maxSize, sBytes; {
  integer scratchLength;
  PMask mask;
  bmBytes = nBytes - nMasks * sizeof(MaskRec);
  DebugAssert(bmBytes > minSize);
  mskBase = (PMask)os_sureCalloc(nMasks, sizeof(MaskRec));
  mskFree = NULL;
  mask = mskBase;
  for (;nMasks > 0; nMasks--) {
    mask->data = (unsigned char *)mskFree;
    mskFree = mask;
    mask++;
    }
  mskLast = mask;
  bmBase = (PBMItem)os_sureCalloc(bmBytes, 1);
  scratchLength = (sBytes + sizeof(BMHeader) - 1) / sizeof(BMHeader);
  bmFree = bmBase;
  bmFree->header.tag = BMFreeTag;
  bmFree->header.length = (bmBytes/sizeof(BMHeader)) - scratchLength;
  bmScratch = (PBMItem)((PBMHeader)bmFree + bmFree->header.length);
  bmScratch->header.tag = BMUsedTag;
  bmScratch->header.length = scratchLength;
  scratchInUse = false;
  bmSize = bmBytes;
  bmUsed = sBytes;
  scratchBytes = sBytes;
  bmMinSize = minSize;
  bmMaxSize = maxSize;
#if STAGE==DEVELOP
  mcCheck = 1;
#endif STAGE==DEVELOP
  }

