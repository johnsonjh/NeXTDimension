/*
 * NDglobals.h
 *
 * Global variables in shared memory between the i860 and host processors.
 *
 * The globals area includes the message queue structures defined in NDmsg.h,
 * and extends the structure.
 *
 * CAUTION:  All elements in this structure must be a multiple of 32 bits in
 * size, and aligned on 32 bit boundries.  This is a hardware imposed
 * limitation.
 */
#include "ND/NDmsg.h"

/*
 * Defs for cursor system.
 */
 
#define CEQSIZE 32
#define XRECT(r) *((int *)r)
#define YRECT(r) *((int *)r+1)

/* Damn well better be an int ! */
enum CEQWhat { CEQMove=1, CEQDisplay, CEQRemove, CEQSet, CEQBright };

typedef struct {
    int		next;		/* next queue element number */
    enum CEQWhat what;		/* type of event */
    int		frame;		/* cursor frame (0 is regular, 1..3 is wait) */
    int		brightness;	/* brightness level 0..63 */
    int		crsrX;
    int		crsrY;
} CEQElement;

/* nd_cursorData is exported so the server can set the cursor image */
extern unsigned int nd_cursorData[];

typedef struct _NDGlobals_
{
	NDMsgQueues	MsgQueues;
	int		ceqHead;
	int		ceqTail;
	int		ceqSize;
	CEQElement	ceq[CEQSIZE];
	unsigned int	cursorData[16];
	unsigned int	cursorMask[16];
} NDGlobals;

#if defined(i860)
#define ND_GLOBALS	((NDGlobals *)MSG_QUEUES)
#else /* Mach side */
#define ND_GLOBALS(ND_var)	((NDGlobals *)MSG_QUEUES(ND_var))
#endif
