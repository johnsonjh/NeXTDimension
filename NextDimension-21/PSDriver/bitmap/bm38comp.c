/*****************************************************************************

	bmcomp38.c
	Low-level 32-bit rect compositing code.

	CONFIDENTIAL
	Copyright (c) 1988 NeXT, Inc. as an unpublished work.
	All Rights Reserved.

	Created: 20Oct89 Ted Cohn

	Modified:
	7/10 pgraff taken from ND composite32

******************************************************************************/

#import PACKAGE_SPECS
#import BITMAP
#import "bm38.h"
#import <nextdev/td.h>

#define ALPHA(s)	((s) & AMASK)
#define NEGALPHA(s)	((~(s)) & AMASK)
#define COLOR(s)	((s) & RGBMASK)

#define MUL(a,b) ((((((a) & rbmask)>>8)*(b) + gamask) & rbmask) | \
		 (((((a) & gamask)*(b)+gamask)>>8) & gamask))


void printro(RectOp *ro)
{
    printf("RectOp:\n");
    printf("op		= %D\n",ro->op);
    printf("width	= %D\n",ro->width);
    printf("height	= %D\n",ro->height);
    printf("srcType	= %D\n",ro->srcType);
    printf("srcPtr	= %X\n",ro->srcPtr);
    printf("srcInc	= %D\n",ro->srcInc);
    printf("srcRowBytes	= %D\n",ro->srcRowBytes);
    printf("srcValue	= %D\n",ro->srcValue);
    printf("dstPtr	= %X\n",ro->dstPtr);
    printf("dstInc	= %D\n",ro->dstInc);
    printf("dstRowBytes	= %D\n",ro->dstRowBytes);
    printf("mask	= %X\n",ro->mask);
    printf("rmask	= %X\n",ro->rmask);
    printf("ltor	= %D\n",ro->ltor);
    printf("ttob	= %D\n",ro->ttob);
}


static int ADDC(unsigned int a, unsigned int b)
{
    unsigned int rb, ga, rbmask=RBMASK, gamask=GAMASK;
    rb = ((a & rbmask)>>8) + ((b & rbmask)>>8);
    ga = (a & gamask) + (b & gamask);
    a = (rb & rbmask) | ((ga & rbmask)>>8);
    a |= a<<1; a |= a<<2; a |= a<<4;
    return (rb<<8) | ga | a;
}

void BMComposite38(RectOp *ro)
{
    int dy, dx, drb, srb, stype, width, si, di;
    volatile unsigned int *dp, *sp;
    unsigned int d, s, mask, rbmask, gamask;

#ifdef i860
    if (ro->op == CLEAR || ro->op == COPY) { BMCopy38(ro); return; }
    if (ro->op == HIGHLIGHT) { BMHighlight38(ro); return; }
    if (ro->op == SOVER) { BMSover38(ro); return; }
    if (ro->op == DISSOLVE) { BMDissolve38(ro); return; }
#endif

    if ((stype = ro->srcType) == SRCCONSTANT) {
	s = ro->srcValue;
    } else {
	sp = (unsigned int *)ro->srcPtr;
	srb = ro->srcRowBytes;
	si = ro->srcInc;
    }
    dy = ro->height;
    drb = ro->dstRowBytes;
    dp = (unsigned int *)ro->dstPtr;
    di = ro->dstInc;
    width = ro->width;
    mask = ro->mask;
    rbmask = RBMASK;
    gamask = GAMASK;
    
    switch(ro->op) {
#ifndef i860
    	case CLEAR:	/* zero */
	    s = 0;
	    stype = SRCCONSTANT;
	    /* Fall through to COPY */
	case COPY:
	    if (stype == SRCBITMAP) {
		if (mask) {
		    if (ro->ltor) {
			for (; dy!=0; dy--, dp+=drb, sp+=srb)
			    for (dx = width; dx!=0; dx--)
				*dp++ = ((*sp++) & mask) | (*dp & (~mask));
		    } else {
			for (; dy!=0; dy--, dp+=drb, sp+=srb)
			    for (dx = width; dx!=0; dx--)
				*dp-- = ((*sp--) & mask) | (*dp & (~mask));
		    }
		} else {
		    if (ro->ltor) {
			for (; dy!=0; dy--, dp+=drb, sp+=srb)
			    /*for (dx = width; dx!=0; dx--)
				*dp++ = *sp++; */
			    LOOP32(width, *dp++ = *sp++);
		    } else {
			for (; dy!=0; dy--, dp+=drb, sp+=srb)
			    /*for (dx = width; dx!=0; dx--)
				*dp-- = *sp--; */
			    LOOP32(width, *dp-- = *sp--);
		    }
		}
	    } else {	/* constant */
		    if (mask) {
			s &= mask;
			for (; dy!=0; dy--, dp+=drb)
			    for (dx = width; dx!=0; dx--)
				*dp++ = s | (*dp & (~mask));
		    } else
			for (; dy!=0; dy--, dp+=drb)
			    /*for (dx = width; dx!=0; dx--)
				*dp++ = s;*/
			    LOOP32(width, *dp++ = s);
		}
	    return;
	case SOVER:	/* dst' = src + dst*(1-srcA) */
	{
	    register unsigned int sa;
	    if (stype == SRCBITMAP)
		for (; dy!=0; dy--, dp+=drb, sp+=srb)
		    for (dx = width; dx!=0; dx--, dp+=di, sp+=si) {
			s = *sp;
			sa = NEGALPHA(s);
			d = *dp;
			*dp = s + MUL(d, sa);
		    }
	    else {
		sa = NEGALPHA(s);
		for (; dy!=0; dy--, dp+=drb)
		    for (dx = width; dx!=0; dx--) {
			d = *dp;
		        *dp++ = s + MUL(d, sa);
		    }
	    }
	    return;
	}
#endif
	case SIN:	/* dst' = src * dstA */
	{
	    register unsigned int da;
	    if (stype == SRCBITMAP)
		for (; dy!=0; dy--, dp+=drb, sp+=srb)
		    for (dx = width; dx!=0; dx--, dp+=di, sp+=si) {
			s = *sp;
			da = ALPHA(*dp);
		        *dp = MUL(s, da);
		    }
	    else
		for (; dy!=0; dy--, dp+=drb)
		    for (dx = width; dx!=0; dx--)
		    {
			da = ALPHA(*dp);
			if (da == 0) d = 0;
			else if (da == 255) d = s;
			else d = MUL(s, da);
			*dp++ = d;
		    }
	    return;
	}
	case SOUT:	/* dst' = src * (1-dstA) */
	{
	    register unsigned int da;
	    if (stype == SRCBITMAP)
		for (; dy!=0; dy--, dp+=drb, sp+=srb)
		    for (dx = width; dx!=0; dx--, dp+=di, sp+=si) {
			s = *sp;
			da = NEGALPHA(*dp);
			*dp = MUL(s, da);
		    }
	    else
		for (; dy>0; dy--, dp+=drb)
		    for (dx = width; dx>0; dx--) {
			da = NEGALPHA(*dp);
			*dp++ = MUL(s, da);
		    }
	    return;
	}
	case SATOP:	/* dst' = src*dstA + dst*(1-srcA) */
	{
	    register unsigned int sa, da;
	    if (stype == SRCBITMAP)
		for (; dy!=0; dy--, dp+=drb, sp+=srb)
		    for (dx = width; dx!=0; dx--, dp+=di, sp+=si) {
			s = *sp;
			sa = NEGALPHA(s);
			if (d = *dp) {  /* clear black dest stays the same */
			    da = ALPHA(d);
			    *dp = MUL(s, da) + MUL(d, sa);
			}
		    }
	    else {
		sa = NEGALPHA(s);
		for (; dy!=0; dy--, dp+=drb)
			for (dx = width; dx!=0; dx--, dp++) {
			if (d = *dp) {  /* clear black dest stays the same */
			    da = ALPHA(d);
			    d = MUL(d, sa);
			    if (da != 255) s = MUL(s, da);
			    *dp = d + s;
			}
		    }
	    }
	    return;
	}
	case DOVER:	/* dst' = src*(1-dstA) + dst */
	{
	    register unsigned int da;
	    if (stype == SRCBITMAP)
		for (; dy!=0; dy--, dp+=drb, sp+=srb)
		    for (dx = width; dx!=0; dx--, dp+=di, sp+=si) {
			s = *sp;
			d = *dp;
			da = NEGALPHA(d);
			*dp = d + MUL(s, da);
		    }
	    else 
		for (; dy!=0; dy--, dp+=drb)
		    for (dx = width; dx!=0; dx--) {
			d = *dp;
			da = NEGALPHA(d);
			*dp++ = d + MUL(s, da);
		    }
	    return;
	}
	case DIN:	/* dst' = dst * srcA */
	{
	    register unsigned int sa;
	    if (stype == SRCBITMAP)
		for (; dy!=0; dy--, dp+=drb, sp+=srb)
		    for (dx = width; dx!=0; dx--, dp+=di, sp+=si) {
			sa = ALPHA(*sp);
			d = *dp;
			*dp = MUL(d, sa);
		    }
	    else {
		sa = ALPHA(s);
		for (; dy!=0; dy--, dp+=drb)
		    for (dx = width; dx!=0; dx--) {
			d = *dp;
			*dp++ = MUL(d, sa);
		    }
	    }
	    return;
	}
	case DOUT:	/* dst' = dst*(1-srcA) */
	{
	    register unsigned int sa;
	    if (stype == SRCBITMAP)
		for (; dy!=0; dy--, dp+=drb, sp+=srb)
		    for (dx = width; dx!=0; dx--, dp+=di, sp+=si) {
			sa = NEGALPHA(*sp);
			d = *dp;
			*dp = MUL(d, sa);
		    }
	    else {
		sa = NEGALPHA(s);
		for (; dy!=0; dy--, dp+=drb)
		    for (dx = width; dx!=0; dx--) {
			d = *dp;
			*dp++ = MUL(d, sa);
		    }
	    }
	    return;
	}
	case DATOP: {	/* dst' = src*(1-dstA) + dst*srcA */
	    register unsigned int sa, da;
	    if (stype == SRCBITMAP)
		for (; dy!=0; dy--, dp+=drb, sp+=srb)
		    for (dx = width; dx!=0; dx--, dp+=di, sp+=si) {
			s = *sp;
			sa = ALPHA(s);
			d = *dp;
			da = NEGALPHA(d);
			*dp = MUL(d, sa) + MUL(s, da);
		    }
	    else {
		sa = ALPHA(s);
		for (; dy!=0; dy--, dp+=drb)
		    for (dx = width; dx!=0; dx--) {
			d = *dp;
			da = NEGALPHA(d);
			*dp++ = MUL(d, sa) + MUL(s, da);
		    }
	    }
	    return;
	}
	case XOR:	/* dst' = src*(1-dstA) + dst*(1-srcA) */
	{
	    register unsigned int sa, da;
	    if (stype == SRCBITMAP)
		for (; dy!=0; dy--, dp+=drb, sp+=srb)
		    for (dx = width; dx!=0; dx--, dp+=di, sp+=si) {
			s = *sp;
			sa = NEGALPHA(s);
			d = *dp;
			da = NEGALPHA(d);
			*dp = MUL(d, sa) + MUL(s, da);
		    }
	    else {
		sa = NEGALPHA(s);
		for (; dy!=0; dy--, dp+=drb)
		    for (dx = width; dx!=0; dx--) {
			d = *dp;
			da = NEGALPHA(d);
			*dp++ = MUL(d, sa) + MUL(s, da);
		    }
	    }
	    return;
	}
	case PLUSD:	/* dstD' = 1-((1-dstD)+^(1-srcD)) */
	    if (stype == SRCBITMAP)
		for (; dy!=0; dy--, dp+=drb, sp+=srb)
		    for (dx = width; dx!=0; dx--, dp+=di, sp+=si) {
			s = *sp;
			s = COLOR(~s) | ALPHA(s);
			d = *dp;
			d = COLOR(~d) | ALPHA(d);
			d = ADDC(s, d);
			*dp = COLOR(~d) | ALPHA(d);
		    }
	    else {
		s = COLOR(~s) | ALPHA(s);
		for (; dy!=0; dy--, dp+=drb)
		    for (dx = width; dx!=0; dx--) {
			d = *dp;
			d = COLOR(~d) | ALPHA(d);
			d = ADDC(s, d);
			*dp++ = COLOR(~d) | ALPHA(d);
		    }
	    }
	    return;
#ifndef i860
	case HIGHLIGHT:	/* swap white and lightgray */
	    for (; dy!=0; dy--, dp+=drb)
		for (dx = width; dx!=0; dx--) {
		    d = COLOR(*dp);
		    if (d == RGB_WHITE)
			*dp++ = RGB_LTGRAY;
		    else if (d == RGB_LTGRAY)
			*dp++ = RGB_WHITE;
		    else dp++;
		}
	    return;
	case DISSOLVE: {	/* dst' = src*f + dst*(1-f) */
	    register unsigned int fn, f;
	    f = ALPHA(ro->delta);
	    fn = NEGALPHA(f);
	    if (ro->ltor) {
		for (; dy!=0; dy--, dp+=drb, sp+=srb)
		    for (dx = width; dx!=0; dx--) {
			s = *sp++;
			d = *dp;
			*dp++ = MUL(s, f) + MUL(d, fn);
		    }
	    } else {
		for (; dy!=0; dy--, dp+=drb, sp+=srb)
		    for (dx = width; dx!=0; dx--) {
			s = *sp--;
			d = *dp;
			*dp-- = MUL(s, f) + MUL(d, fn);
		    }
	    }
	    return;
	}
#endif
	case PLUSL: {	/* dstD' = dstD +^ srcD */
	    if (stype == SRCBITMAP)
		for (; dy!=0; dy--, dp+=drb, sp+=srb)
		    for (dx = width; dx!=0; dx--, dp+=di, sp+=si) {
			s = *sp;
			d = *dp;
			*dp = ADDC(s, d);
		    }
	    else
		for (; dy!=0; dy--, dp+=drb)
		    for (dx = width; dx!=0; dx--) {
			d = *dp;
			*dp++ = ADDC(s, d);
		    }
	    return;
	}
	default:
	    CantHappen();
    }
}



