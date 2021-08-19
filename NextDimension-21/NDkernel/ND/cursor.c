/*****************************************************************************

    cursor.c
    Cursor routines for NDkernel
    
    CONFIDENTIAL
    Copyright 1990 NeXT Computer, Inc. as an unpublished work.
    All Rights Reserved.

    04Sep90 Ted Cohn
    
    Modifications:

******************************************************************************/

#include <mach_types.h>
#include "package_specs.h"
#include "bintree.h"
#include "ND/NDGlobals.h"
#include "ND/NDreg.h"

/* Private */
static int nd_cursorFrame;		/* Which cursor image to display */
static Bounds nd_saveRect;		/* Current save bounds */
static unsigned int nd_saveData[256];	/* 32-bit saved screen data */

/* Globals */
int nd_flushCEQ;			/* semaphore for postscript sync */
unsigned int nd_cursorData[1024];	/* Four 32-bit cursor images */
Bounds fb_rect;
unsigned int *fb_addr, fb_size;

/* Called at the dawn of time by i860_init.c */
void InitCEQ()
{
    int i;
    volatile NDGlobals *g = ND_GLOBALS;

    /* Initialize the low-level cursor event queue */
    g->ceqSize = CEQSIZE;
    bzero(g->ceq, g->ceqSize*sizeof(CEQElement));
    for (i = g->ceqSize; --i >= 0; )
	g->ceq[i].next = i+1;
    g->ceq[g->ceqSize - 1].next = 0;
    g->ceqTail = g->ceqHead = 0;
    *((int *)&nd_saveRect) = 0;
    *((int *)&nd_saveRect+1) = 0;
}


void DisplayCursor(Bounds *cursorRect)
{
    int i, j, width, cursRow, vramRow;
    volatile unsigned int *vramPtr, *savePtr, *cursPtr;
    unsigned int s, d, f, rbmask=0xFF00FF00, gamask=0xFF00FF;
    
    if ( fb_addr == (unsigned int *)0 ) return;
    nd_saveRect = *cursorRect;
    if (nd_saveRect.minx < fb_rect.minx) nd_saveRect.minx = fb_rect.minx;
    if (nd_saveRect.maxx > fb_rect.maxx) nd_saveRect.maxx = fb_rect.maxx;
    if (nd_saveRect.miny < fb_rect.miny) nd_saveRect.miny = fb_rect.miny;
    if (nd_saveRect.maxy > fb_rect.maxy) nd_saveRect.maxy = fb_rect.maxy;

    width = nd_saveRect.maxx - nd_saveRect.minx;
    vramRow = NDROWBYTES>>2;
    vramPtr = fb_addr + (vramRow * (nd_saveRect.miny - fb_rect.miny)) +
			(nd_saveRect.minx - fb_rect.minx);
    vramRow -= width;
    cursPtr = &nd_cursorData[nd_cursorFrame<<8];
    cursPtr += (nd_saveRect.miny - cursorRect->miny) * CURSORWIDTH +
		nd_saveRect.minx - cursorRect->minx;
    savePtr = nd_saveData;
    cursRow = CURSORWIDTH - width;
    
    for (i = nd_saveRect.maxy-nd_saveRect.miny; i>0; i--) {
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


void RemoveCursor()
{
    int i, j, width, vramRow;
    volatile unsigned int *vramPtr, *savePtr;
    
    if ( fb_addr == (unsigned int *)0 ) return;
    savePtr = nd_saveData;
    vramRow = NDROWBYTES>>2;
    vramPtr = fb_addr + (vramRow * (nd_saveRect.miny - fb_rect.miny)) +
			(nd_saveRect.minx - fb_rect.minx);
    width = nd_saveRect.maxx - nd_saveRect.minx;
    vramRow -= width;
    for (i = nd_saveRect.maxy - nd_saveRect.miny; --i>=0; ) {
	for (j = width; --j>=0; ) *vramPtr++ = *savePtr++;
	vramPtr += vramRow;
    }
}


void SetCursor(unsigned int *data, unsigned int *mask)
{
    int i, j;
    unsigned int d, m, e, f;
    volatile unsigned int *cp;
    
    if ( fb_addr == (unsigned int *)0 ) return;
    cp = &nd_cursorData[nd_cursorFrame<<8];
    for (i=16; i>0; i--) {
	m = *mask++;			/* Get mask data and inc ptr */
        d = *data++;			/* Get grey data and convert */
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
}

/* ProcessCEQ dequeues and processes each entry in the low-level cursor
 * event queue (aka, CEQ).  This is called by CursorTask when a VBL interrupt
 * occurs and FlushCEQ when PostScript needs to synchronize cursor drawing.  
 */
void ProcessCEQ()
{
    volatile NDGlobals *g = ND_GLOBALS;
    volatile CEQElement *theTail;

    if (g->ceqTail == g->ceqHead) return;	/* i.e., nothing to process */
    theTail = &g->ceq[g->ceqTail];
    /* Process items until queue exhausted */
    while (g->ceqTail != g->ceqHead) {
	switch(theTail->what) {
	    case CEQMove:
		nd_cursorFrame = theTail->frame;
		RemoveCursor();
		DisplayCursor((Bounds *)&theTail->crsrX);
		break;
	    case CEQDisplay:
		nd_cursorFrame = theTail->frame;
		DisplayCursor((Bounds *)&theTail->crsrX);
		break;
	    case CEQRemove:
		RemoveCursor();
		break;
	    case CEQBright:
		//SetBrightness(theTail->brightness);
		break;
	    case CEQSet:
		nd_cursorFrame = theTail->frame;
		SetCursor((unsigned int *)g->cursorData,
		    (unsigned int *)g->cursorMask);
		break;
	    default:
		break;
	}
	theTail = &g->ceq[g->ceqTail = theTail->next];
    }
}

/* CursorTask is called every vertical retrace interrupt by hardclock() */
void CursorTask()
{
    if (!nd_flushCEQ)
	ProcessCEQ();
}


/* FlushCEQ is called by the server to synchronize cursor drawing usually
 * before PostScript wants to Remove or Display the cursor.
 */
void FlushCEQ()
{
    nd_flushCEQ++;
    ProcessCEQ();
    nd_flushCEQ--;
}




