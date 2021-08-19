/*****************************************************************************

    support.c
    hacks galore...
    
    CONFIDENTIAL
    Copyright 1990 NeXT Computer, Inc. as an unpublished work.
    All Rights Reserved.

    01Aug90 Ted Cohn
    
    Modifications:

******************************************************************************/

#import "package_specs.h"
#import "devicetypes.h"
#import "except.h"
#import "fp.h"
#import "bintree.h"
#import "server.h"

static DevScreen myScreen;
static DevHalftone myHalftone;
static unsigned char checkerboard[4] = {64, 191, 255, 128};
static char *patternPool; /* Blind pointer to storage pool for Patterns */
Pattern *blackpattern, *whitepattern;
_Exc_Buf *_Exc_Header;

void PFree(Pattern *pat)
{
    if (pat->permanent)
	return;
    os_freeelement(patternPool, pat);
}

Pattern *PNew()
{
    Pattern *pat;
    
    pat = (Pattern *) os_newelement(patternPool);
    pat->type = PATTERN;
    return pat;
}

Pattern *PNewColorAlpha(unsigned int color, unsigned char alpha,
    DevHalftone *halftone)
{
    Pattern *pat;
    
    pat = PNew();
    pat->alpha = alpha;
    *((unsigned int *)&pat->color) = color;
    pat->phase.x = pat->phase.y = 0;
    pat->permanent = 1;
    pat->halftone = halftone;
    return pat;
}

void CantHappen()
{
  printf("Fatal system error (address unknown)\n");
  os_abort();
}

public charptr EXPAND(current, n, size)  charptr current; integer n, size;
{				/* NB! this proc allocates from unix
				 * subroutine package, not VM */
  charptr c;

  if ((c = (charptr) os_realloc(
    (char *) current, (long int)(n * size))) == NIL)
    CantHappen(); /* Ted - used to be VMERROR()*/
  return c;
}				/* end of EXPAND */

void os_raise(code, msg)
  int code; char *msg;
{
  register _Exc_Buf *EBp = _Exc_Header;

  if (EBp == 0)		/* uncaught exception */
    {
    os_printf("Uncaught exception: %d  %s\n", code, (msg == NIL)? "" : msg);
    CantHappen();
    }

  EBp->Code = code;
  EBp->Message = msg;
  _Exc_Header = EBp->Prev;
  longjmp(EBp->Environ, 1);
}

void PSLimitCheck() {RAISE(0, "limitcheck");}
void PSRangeCheck() {RAISE(0, "rangecheck");}

public procedure TfmPCd(c, m, ct)  Cd c;  register PMtx m;  Cd *ct;
{
register real s;
if (RealEq0(m->a))
  s = c.y * m->c;
else {
  s = c.x * m->a;
  if (!RealEq0(m->c))
    s += c.y * m->c;
  }
ct->x = s + m->tx;
if (RealEq0(m->b))
  s = c.y * m->d;
else {
  s = c.x * m->b;
  if (!RealEq0(m->d))
    s += c.y * m->d;
  }
ct->y = s + m->ty;
} /* end of TfmPCd */

integer GCD(u, v)  integer u, v;
{integer t; while (v != 0) {t = u % v; u = v; v = t;} return u;}

static void IntInterval(iNew, iresult)        /* result = intersect(iNew, iresult) */
  register DevInterval *iNew, *iresult;
{
  if (iNew->l > iresult->l)
    iresult->l = iNew->l;
  if (iNew->g < iresult->g)
    iresult->g = iNew->g;
} /* IntInterval */

static integer YDelta(edgeA, edgeB, yRange)
 /* when edges don't cross return -1 if edgeA<=edgeB else 0 if edgeB<edgeA,
 else return +ive dy to y-value beyond which they are equal or reversed */
  register DevTrapEdge *edgeA, *edgeB;
  register int yRange;
{
  register int dy;

  if (yRange == 1)
    return((edgeA->xl <= edgeB->xl) ? -1 : 0);
  if (edgeA->xl < edgeB->xl) {
    if (edgeA->xg > edgeB->xg)
      goto FindDelta;
    return(-1);
  }
  if (edgeA->xl > edgeB->xl) {
    if (edgeA->xg < edgeB->xg)
      goto FindDelta;
    return(0);
  }
  return((edgeA->xg <= edgeB->xg) ? -1 : 0);
FindDelta:  /* here we know they cross */
  if (yRange == 2)
    return(1);
  if ((yRange == 3) || (edgeB->dx == edgeA->dx))
    if ((edgeA->xl <= edgeB->xl) ?
        (edgeA->ix <= edgeB->ix) : (edgeA->ix > edgeB->ix))
      dy = yRange - 1; /* ix's have same ordering as xl's */
    else
      dy = 1;
  if (yRange < 4)
    return (dy);
  if (edgeA->xl == edgeA->xg)
    dy = (FixInt(edgeA->xg) - edgeB->ix) / edgeB->dx;
  else if (edgeB->xl == edgeB->xg)
    dy = (FixInt(edgeB->xg) - edgeA->ix) / edgeA->dx;
  else if (edgeB->dx == edgeA->dx)
    return (dy); /* calculated above */
  else
    dy = (edgeA->ix - edgeB->ix) / (edgeB->dx - edgeA->dx);
  dy += 1;
  if (dy <= 0)
    return(1);
  if (dy >= yRange)
    return(yRange - 1);
  return (dy);
} /* YDelta */

static void CopyTrimmedEdge(eOld, eNew, yOld, yNew)
  register DevTrapEdge *eOld, *eNew;
  register DevInterval *yOld, *yNew;
{
  register int dy;

  Assert
    ((yNew->l >= yOld->l) && (yNew->g <= yOld->g) && (yNew->g > yNew->l));
  if (eOld->xl == eOld->xg) {	/* it's a vertical edge */
    eNew->xl = eOld->xl;
    eNew->xg = eOld->xg;
    return;
  }
  eNew->ix = eOld->ix;
  eNew->dx = eOld->dx;
  dy = yNew->l - yOld->l;
  if (dy == 0)	   /* use old first line */
    eNew->xl = eOld->xl;
  else if (yOld->g - yNew->l == 1)	/* use old last line */
    eNew->xl = eOld->xg;
  else {	   /* bring l up to yNew->l */
    eNew->ix += dy * eNew->dx;
    eNew->xl = FTrunc(eNew->ix - eNew->dx);
  }
  if (yNew->g - yNew->l == 1)
    return;	   /* 1 line result */
  if (yNew->g < yOld->g) {	/* trim down to yNew->g */
    dy = yNew->g - yNew->l - 2;
    Assert(dy >= 0);
    eNew->xg = FTrunc(eNew->ix + dy * eNew->dx);
  } else
    eNew->xg = eOld->xg;
}

BBoxCompareResult BoxTrapCompare(figbb, clipt, inner, outer, rt)
  register DevBounds *figbb;
  register DevInterval *inner, *outer;
  register DevTrap *clipt, *rt;
/* BoxTrapCompare returns the result of comparing the figure bounding
   box with the clipping trapezoid.  Returns "inside" iff "figbb" is
   completely inside "clipt"; returns "outside" iff they are disjoint;
   and returns "overlap" iff "figbb" is partially inside "clipt" and
   partially outside "clipt". If (rt) interpolation will be made
   on diagonal edges of the clipt, storing the result in *rt, where
   necessary to refine the accuracy of the result. In the case that
   "overlap" is returned, a complete reduced trapezoid will be stored.
   A reduced trapezoid is the smallest possible trapezoid whose
   intersection with figbb is the same as clipt's.  (In other cases,
   rt may be altered but meaningless.) */
{
  register BBoxCompareResult result;

  if ((clipt->y.l >= clipt->y.g) || /* null trap (causes problems later!) */
   (clipt->y.l >= figbb->y.g) || (clipt->y.g <= figbb->y.l))
    return (outside); /* disjoint in y */
  if (clipt->l.xl < clipt->l.xg) {
    inner->l = clipt->l.xg;
    outer->l = clipt->l.xl;
  } else {
    inner->l = clipt->l.xl;
    outer->l = clipt->l.xg;
  }
  if (outer->l >= figbb->x.g)
    return (outside);
  if (clipt->g.xl < clipt->g.xg) {
    inner->g = clipt->g.xl;
    outer->g = clipt->g.xg;
  } else {
    inner->g = clipt->g.xg;
    outer->g = clipt->g.xl;
  }
  if (outer->g <= figbb->x.l)
    return (outside);
  /* figbb is not trivially outside clipt */
  if (!rt) /* no reduction is desired */
    return ((
      (inner->l <= figbb->x.l) &&
      (inner->g >= figbb->x.g) &&
      (clipt->y.l <= figbb->y.l) &&
      (clipt->y.g >= figbb->y.g)) ? inside : overlap);
  /* else get the final y result */
  if (clipt->y.l <= figbb->y.l) {
    result = inside;
    rt->y.l = figbb->y.l;
  } else {
    result = overlap;
    rt->y.l = clipt->y.l;
  }
  if (clipt->y.g >= figbb->y.g)
    rt->y.g = figbb->y.g;
  else {
    result = overlap;
    rt->y.g = clipt->y.g;
  }
  if ((result == inside) &&
    (inner->l <= figbb->x.l) && (inner->g >= figbb->x.g))
      return (inside); /* y & x both inside without reduction */
  CopyTrimmedEdge(&clipt->l, &rt->l, &clipt->y, &rt->y);
  CopyTrimmedEdge(&clipt->g, &rt->g, &clipt->y, &rt->y);
  if ((rt->y.l != clipt->y.l) || (rt->y.g != clipt->y.g)) {
  /* y reduction occurred, recompute inner, outer & x-outside check */
    if (rt->l.xl < rt->l.xg) {
      inner->l = rt->l.xg;
      outer->l = rt->l.xl;
    } else {
      inner->l = rt->l.xl;
      outer->l = rt->l.xg;
    }
    if (outer->l >= figbb->x.g)
      return (outside);
    if (rt->g.xl < rt->g.xg) {
      inner->g = rt->g.xl;
      outer->g = rt->g.xg;
    } else {
      inner->g = rt->g.xg;
      outer->g = rt->g.xl;
    }
    if (outer->g <= figbb->x.l)
      return (outside);
    /* now reduced x either overlaps or is inside */
    if ((result == inside) && /* in y, see if x is also */
      (inner->l <= figbb->x.l) && (inner->g >= figbb->x.g))
        return (inside);
  }
  /* now we have irreducible overlap, but may be able to reduce x to figbb */
  if (inner->l <= figbb->x.l)
    rt->l.xl = rt->l.xg = figbb->x.l;
  if (inner->g >= figbb->x.g)
    rt->g.xl = rt->g.xg = figbb->x.g;
  return (overlap);
} /* BoxTrapCompare */

integer TrapTrapInt(t0, t1, yptr, callback, callbackarg)
  DevTrap *t0, *t1;
  DevInterval *yptr; /* optional range restriction */
  void (*callback) ();/* takes a (DevTrap *), and (char *) */
  char *callbackarg;
{
  register int yRange, dy, ll;
  DevTrap t[2];
  DevInterval y, yl;

  /* find the common y range */
  if (yptr) {
    y = *yptr;
    IntInterval(&(t0->y), &y);
  } else 
    y = t0->y;
  IntInterval(&(t1->y), &y);
  yRange = y.g - y.l;
  if (yRange <= 0)
    return;           /* disjoint in y */

  /* copy edges to our storage, trim them to common y range */
  CopyTrimmedEdge(&(t0->l), &t[0].l, &(t0->y), &y);
  CopyTrimmedEdge(&(t0->g), &t[0].g, &(t0->y), &y);
  CopyTrimmedEdge(&(t1->l), &t[1].l, &(t1->y), &y);
  CopyTrimmedEdge(&(t1->g), &t[1].g, &(t1->y), &y);
  /* define indices of inner edges at y.l */
  dy = YDelta(&t[1].g, &t[0].l, yRange);
  switch (dy) {
  case -1:
    return;
  case 0:
    break;
  default:
    goto CrossingFound;
  }
  dy = YDelta(&t[0].g, &t[1].l, yRange);
  switch (dy) {
  case -1:
    return;
  case 0:
    break;
  default:
    goto CrossingFound;
  }
  dy = YDelta(&t[0].l, &t[1].l, yRange);
  if (dy>0)
    goto CrossingFound;
  ll = -dy; /* remember index of innermost lesser edge */
  dy = YDelta(&t[1].g, &t[0].g, yRange);
  if (dy>0)
    goto CrossingFound;
  /* else single trap defined by inner edges */
  if (ll != -dy)
    t[ll].g = t[-dy].g;
  t[ll].y = y;
#if (STAGE == DEVELOP)
  OKTrap(&t[ll], false);
#endif
  (*callback)(&t[ll], callbackarg);
  return;
CrossingFound:
  /* though might avoid if (yRange>2); divide at dy and recurse */
  t[0].y = t[1].y = y;
  yl.l = y.l;
  yl.g = y.l + dy;
  TrapTrapInt(&t[0], &t[1], &yl, callback, callbackarg);
  y.l = yl.g;
  TrapTrapInt(&t[0], &t[1], &y, callback, callbackarg);
} /* TrapTrapInt */

#define fixedScale 65536.0	/* i=15, f=16, range [-32768, 32768) */
Fixed pflttofix(pf)
  float *pf;
{
  float f = *pf;
  if (f >= FixedPosInf / fixedScale) return FixedPosInf;
  if (f <= FixedNegInf / fixedScale) return FixedNegInf;
  return (Fixed) (f * fixedScale);
}

void SupportInit()
{
    patternPool = (char *) os_newpool(sizeof(Pattern), 0, 0);
    myScreen.width = 2;
    myScreen.height = 2;
    myScreen.thresholds = checkerboard;
    myHalftone.white = &myScreen;
    myHalftone.priv = NULL; /* Continues to be null until gray is used */
    whitepattern = PNewColorAlpha(0xffffffff, 0xff, &myHalftone);
    blackpattern = PNewColorAlpha(0x0, 0xff, &myHalftone);
}

#ifdef i860
mach_error( char *s, int i )
{
	printf( "mach_error: %s (%d)\n", s, i );
}
#endif

