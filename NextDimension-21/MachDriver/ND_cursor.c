/*
 * Copyright 1990 NeXT, Inc.
 *
 * Prototype NextDimension loadable kernel server.
 *
 * This module contains the code which sends a message out the reply port
 * on receipt of an interrupt from the NextDimension board.
 *
 * 20-Jun-90 tcohn at NeXT
 *	Fixed wait cursor cache.
 */
#include "ND_patches.h"			/* Must be FIRST! */
#include <nextdev/slot.h>
#include <nextdev/evio.h>
#include <next/pcb.h>
#include <next/scr.h>	/* Def of BRIGHT_MAX... */
#include "ND_var.h"
#include "ND/NDGlobals.h"

extern unsigned char ND_GammaTable[];
extern ND_var_t ND_var[SLOTCOUNT];
extern EvVars *evp;
#define CURSORWIDTH 16
#define CURSORHEIGHT 16

void ND_SetCursor(evioScreen *, int *, int *, int, int, int);
void ND_RemoveCursor(evioScreen *, Bounds);
void ND_DisplayCursor(evioScreen *, Bounds, int);
void ND_MoveCursor(evioScreen *, Bounds, Bounds, int);
void ND_SetBrightness(evioScreen *, int);

#ifndef CURSOR_i860
static unsigned int waitCursor1[256], waitCursor2[256], waitCursor3[256];
static unsigned int *waitCursors[]={0, waitCursor1, waitCursor2, waitCursor3};
#endif
static int waitCache;

#define CLAMPWAIT(F) (F)=((F)<0?0:((F)>3?3:(F)))

#ifdef CURSOR_i860
void ND_PushCEQ(volatile NDGlobals *g, volatile CEQElement *cep)
{
    volatile CEQElement *theHead;

    theHead = &g->ceq[g->ceqHead];
    if (theHead->next != g->ceqTail) {
	theHead->what = cep->what;
	theHead->frame = cep->frame;
	theHead->brightness = cep->brightness;
	theHead->crsrX = cep->crsrX;
	theHead->crsrY = cep->crsrY;
	g->ceqHead = theHead->next;
    } else ; /* else drop event on the floor! */
}
#endif

kern_return_t ND_RegisterThyself(
		port_t	ND_port,
		int	unit,
		int	NXSDevice,
		int	Min_X,
		int	Max_X,
		int	Min_Y,
		int	Max_Y)
{
	ND_var_t *sp;
	evioScreen es;
	extern int ev_register_screen(evioScreen *);
	/*
	 * Ensure that arg makes sense.
	 */
	if ( UNIT_INVALID( unit ) )
		return KERN_INVALID_ARGUMENT;
	sp = &ND_var[UNIT_TO_INDEX(unit)];
	if ( sp->present == FALSE )
		return KERN_INVALID_ADDRESS;

	waitCache = 0;
	es.device = NXSDevice;
	es.bounds.minx = Min_X;
	es.bounds.maxx = Max_X;
	es.bounds.miny = Min_Y;
	es.bounds.maxy = Max_Y;
	es.SetCursor = ND_SetCursor;
	es.DisplayCursor = ND_DisplayCursor;
	es.RemoveCursor = ND_RemoveCursor;
	es.MoveCursor = ND_MoveCursor;
	es.SetBrightness = ND_SetBrightness;
	es.priv = unit;	/* 0, 2, 4, or 6 */
	sp->ev_token = ev_register_screen(&es);
	return KERN_SUCCESS;
}

/* Called internally, on shutdown or death of the owner port. */
void ND_UnregisterThyself( ND_var_t *sp )
{
    if (sp->ev_token)
	ev_unregister_screen(sp->ev_token);
    sp->ev_token = 0;
}

void ND_SetCursor(evioScreen *es, int *data, int *mask, int drow, int mrow,
    int waitFrame)
{
    /* If we hit our wait cursor cache, don't need to set cursor */
    CLAMPWAIT(waitFrame);
    if (waitFrame)
	if (waitCache & (1<<waitFrame))
	    return;				/* Hit Cache */
	else
	    waitCache |= (1<<waitFrame);	/* Mark Cache */
#ifdef CURSOR_i860
    {
	CEQElement ce;
	volatile NDGlobals *g;
    
	g = ND_GLOBALS(&ND_var[UNIT_TO_INDEX((int)es->priv)]);
	bcopy(data, &g->cursorData, 64); 
	bcopy(mask, &g->cursorMask, 64);  
	ce.what = CEQSet;
	ce.frame = waitFrame;
	ND_PushCEQ(g, &ce);
    }
#else
    {
	int i, j;
	unsigned int d, m, e, f;
	volatile unsigned int *cp;
	
	cp = (waitFrame ? waitCursors[waitFrame] : evp->nd_cursorData);
	mrow >>= 2;
	drow >>= 2;
	for (i=16; i>0; i--) {
	    m = *mask; mask += mrow;	/* Get mask data and inc ptr */
	    d = *data; data += drow;	/* Get grey data and convert */
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
#endif
}

void ND_RemoveCursor(evioScreen *es, Bounds cursorRect)
{
#ifdef CURSOR_i860
    CEQElement ce;
    ce.what = CEQRemove;
    ND_PushCEQ(ND_GLOBALS(&ND_var[UNIT_TO_INDEX((int)es->priv)]), &ce);
#else
    int i, j, width, vramRow;
    volatile unsigned int *vramPtr;
    volatile unsigned int *savePtr;

    vramRow = 1152;
    vramPtr = (unsigned int *) ND_var[es->priv >> 1].fb_addr;
    vramPtr += (vramRow * (evp->saveRect.miny - es->bounds.miny)) +
	       (evp->saveRect.minx - es->bounds.minx);
    savePtr = evp->saveData;
    width = evp->saveRect.maxx-evp->saveRect.minx;
    vramRow -= width;
    for (i = evp->saveRect.maxy-evp->saveRect.miny; i>0; i--) {
	for (j = width; j>0; j--) *vramPtr++ = *savePtr++;
	vramPtr += vramRow;
    }
#endif
}

void ND_DisplayCursor(evioScreen *es, Bounds cursorRect, int waitFrame)
{
#ifdef CURSOR_i860
    CEQElement ce;
    CLAMPWAIT(waitFrame);
    ce.what = CEQDisplay;
    ce.frame = waitFrame;
    ce.crsrX = XRECT(&cursorRect);
    ce.crsrY = YRECT(&cursorRect);
    ND_PushCEQ(ND_GLOBALS(&ND_var[UNIT_TO_INDEX((int)es->priv)]), &ce);
#else
    int i, j, width, cursRow, vramRow;
    unsigned int s, d, f, rbmask=0xFF00FF00, gamask=0xFF00FF;
    Bounds bounds = es->bounds;
    volatile unsigned int *vramPtr;
    volatile unsigned int *savePtr;
    unsigned int *cursPtr;

    CLAMPWAIT(waitFrame);
    evp->saveRect = cursorRect;
    /* Clip saveRect against the screen bounds */
    if (evp->saveRect.miny < bounds.miny) evp->saveRect.miny = bounds.miny;
    if (evp->saveRect.maxy > bounds.maxy) evp->saveRect.maxy = bounds.maxy;
    if (evp->saveRect.minx < bounds.minx) evp->saveRect.minx = bounds.minx;
    if (evp->saveRect.maxx > bounds.maxx) evp->saveRect.maxx = bounds.maxx;

    vramPtr = (unsigned int *) ND_var[es->priv >> 1].fb_addr;
    vramRow = 1152;	/* longs per scanline */
    vramPtr += (vramRow * (evp->saveRect.miny - bounds.miny)) +
	       (evp->saveRect.minx - bounds.minx);
    width = evp->saveRect.maxx-evp->saveRect.minx;
    vramRow -= width;
    cursPtr = (waitFrame ? waitCursors[waitFrame] : evp->nd_cursorData);
    cursPtr += (evp->saveRect.miny - cursorRect.miny) * CURSORWIDTH +
	       (evp->saveRect.minx - cursorRect.minx);
    savePtr = evp->saveData;
    cursRow = CURSORWIDTH - width;
    
    for (i = evp->saveRect.maxy-evp->saveRect.miny; i>0; i--) {
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
#endif
}

void ND_MoveCursor(evioScreen *es, Bounds oldCursorRect, Bounds cursorRect,
    int waitFrame)
{
#ifdef CURSOR_i860
    CEQElement ce;
    CLAMPWAIT(waitFrame);
    ce.what = CEQMove;
    ce.frame = waitFrame;
    ce.crsrX = XRECT(&cursorRect);
    ce.crsrY = YRECT(&cursorRect);
    ND_PushCEQ(ND_GLOBALS(&ND_var[UNIT_TO_INDEX((int)es->priv)]), &ce);
#else
    CLAMPWAIT(waitFrame);
    ND_RemoveCursor(es, oldCursorRect);
    ND_DisplayCursor(es, cursorRect, waitFrame);
#endif
}

#ifdef PROTOTYPE
void ND_SetBrightness(evioScreen *es, int level)
{
    /* Adjust gamma table on nd board according to desired brightness level */
    ND_var_t *sp;
    int scale;
    unsigned int *raddr, *rreg, *rpal, b, index;

    sp = &ND_var[es->priv >> 1];
    
    raddr = (unsigned int *)ADDR_DAC_ADDR_PORT(sp); // Set address
    rreg =  (unsigned int *)ADDR_DAC_REG_PORT(sp); // Set at current address
    rpal =  (unsigned int *)ADDR_DAC_PALETTE_PORT(sp);
    // Set palette w/ auto-incr
    
    // Load up the 3 DACs with blink off, unmasked.
    *raddr = 0x04040404; *rreg = 0xffffff00;
    *raddr = 0x05050505; *rreg = 0;
    *raddr = 0x06060606; *rreg = 0x40404000;
    *raddr = 0x07070707; *rreg = 0;
    
    // Load a standard palette, gamma 2.3
    *raddr = 0x00000000;	// Select start of palatte
    scale = (level*64)/BRIGHT_MAX;
    for (index = 0; index < 256; index++)
    {
        b = (unsigned int)((ND_GammaTable[index]*scale)>>6);
	*rpal = (b << 24) | (b << 16) | (b << 8);
    }	 
}

#else /* ! PROTOTYPE */

void ND_SetBrightness(evioScreen *es, int level)
{
/*
#ifdef CURSOR_i860
    CEQElement ce;
    ce.what = CEQBright;
    ce.brightness = level;
    ND_PushCEQ(ND_GLOBALS(&ND_var[UNIT_TO_INDEX((int)es->priv)]), &ce);
#else
*/
    /* Adjust gamma table on nd board according to desired brightness level */
    int scale;
    volatile unsigned char *raddr;
    unsigned int b, index;

    raddr = (volatile unsigned char *) ND_var[es->priv >> 1].bt_addr;
    
    scale = (level*64)/BRIGHT_MAX;
    
    /* Load a ramp in the palatte. First 256 loactions */
    for (index = 0; index < 256; index++)
    {
        b = (unsigned int)((ND_GammaTable[index]*scale)>>6);
	raddr[0] = index & 0xFF;
	raddr[4] = (index & 0xFF00) >> 8;
	    raddr[0xC] = b;
	    raddr[0xC] = b;
	    raddr[0xC] = b;
    }
/* #endif */
}
#endif /* ! PROTOTYPE */
