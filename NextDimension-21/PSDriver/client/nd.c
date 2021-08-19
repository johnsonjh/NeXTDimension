/*****************************************************************************

    nd.c
    NextDimension Loadable PostScript Driver

    CONFIDENTIAL
    Copyright 1990 NeXT Computer, Inc. as an unpublished work.
    All Rights Reserved.

    04Oct89 Ted Cohn

    Modifications:
	19Oct89 Ted	Received first color prototype board!  Major hacking.
	02Feb90 Terry	Added NDWakeup, NDSetupImageArgs, NDSetupMark,
			and NDSetupPattern from windowdevice.c
	06Feb90 Ted	Changed over to enchanced bitmap structures.
			Cleanup, compiling.
	07Feb90 Ted	Added ChooseFormat, ExportBitmap and NDConvert2to32.
	12Feb90 Jack	eliminate halftones & NDSetupPattern
	20Mar90 Terry	Move device setup routines into nddev.c
	02Apr90 Terry	Added BitmapInfo to NDNewBitmap, removed NDCreateBitmap
 	03Apr90 Terry	Changed over to using exported ps interfaces
 	16Apr90 Jack	fix TrapTrapInt crash by initializing ttinfo.mrec
	23May90 Ted	Upgraded to use new protocol (bags, hooks, etc).
	22Jul90 Ted	New API improvements, NXCursorInfo. Finally
			eliminated nextdev/evio.h and evp!

******************************************************************************/

#import <mach.h>
#import <mach_init.h>
#import <servers/netname.h>
#import <ND/NDlib.h>
#import <math.h>
#import <sys/file.h>
#import <sys/ioctl.h>
#import <sys/types.h>
#import <sys/stat.h>
#import "package_specs.h"
#import "customops.h"
#import "devicetypes.h"
#import "devpattern.h"
#import "bintree.h"
#import "bitmap.h"
#import "nd.h"
#import "remote.h"

void NDComposite(CompositeOperation *, Bounds *);
void NDDisplayCursor(NXDevice *, NXCursorInfo *);
void NDFreeWindow(NXBag *);
void NDHook(NXHookData *);
void NDInitScreen(NXDevice *);
void NDMark(NXBag *, int, MarkRec *, Bounds *, Bounds *);
void NDMoveWindow(NXBag *, short, short, Bounds *, Bounds *);
void NDNewAlpha(NXBag *bag);
void NDNewWindow(NXBag *, Bounds *, int, int, int);
void NDPing(NXDevice *);
void NDPromoteWindow(NXBag *, Bounds *, int, int, DevPoint);
void NDRegisterScreen(NXDevice *);
void NDRemoveCursor(NXDevice *, NXCursorInfo *);
void NDSyncCursor(NXDevice *, int);
void NDSetCursor(NXDevice *, NXCursorInfo *, LocalBitmap *);
int  NDWindowSize(NXBag *);

static const NXProcs ndProcs = {
    NDComposite,
    NDDisplayCursor,
    NDFreeWindow,
    NDHook,
    NDInitScreen,
    NDMark,
    NDMoveWindow,
    NDNewAlpha,
    NDNewWindow,
    NDPing,
    NDPromoteWindow,
    NDRegisterScreen,
    NDRemoveCursor,
    NDSetCursor,
    NDSyncCursor,
    NDWindowSize
};

port_t nd_dev_port;		/* port to NextDimension Mach Driver */
static int shouldfork = 1;	/* Set to fork off server. Debugging only */
static Bitmap *HackBM;
static NXDevice *HackDevice;

int NDStart(NXDriver *driver)
{
    int i;
    kern_return_t r;
    unsigned int slotbits = 0;
    
    if ( (r = ND_Load_MachDriver()) != KERN_SUCCESS ){
    	kern_loader_error( "NDDriver: ND_Load_MachDriver", r );
	return 1;
    }
    r = netname_look_up(name_server_port, "", "NextDimension", &nd_dev_port);
    if (r != KERN_SUCCESS) {
	mach_error("NDDriver: netname_lookup: ND timeout", r);
	ND_Remove_MachDriver();
	return 1;
    }
    if ((r = ND_GetBoardList( nd_dev_port, &slotbits )) != KERN_SUCCESS) {
    	mach_error( "NDDriver: ND_GetBoardList", r );
	ND_Remove_MachDriver();
	return 1;
    }
    if (slotbits == 0) {
#if defined(DEBUG)
	/* Bug 8360:
	 * It is not an error for a system to have no NextDimension boards.
	 */
    	os_fprintf(os_stderr, "NDDriver: no NextDimension devices found.\n");
#endif
	ND_Remove_MachDriver();
	return 1;
    }
    driver->name = "NextDimension";
    driver->procs = (NXProcs *)&ndProcs;   
    /* Post a monitor to DPS for each ND board present in the system. */
    for (i=0; slotbits && i<32; ++i) {
    	if ((slotbits & 1) == 1)
	    NXRegisterScreen(driver, i, 0, ND_WIDTH, ND_HEIGHT);
	slotbits >>= 1;
    }
    return 0;
}

void NDComposite(CompositeOperation *cop, Bounds *dstBounds)
{
    BMCompOp bcop;
    Bitmap *psb;	/* public source bitmap */
    Bitmap *db;		/* destination bitmap */

    psb = NULL;
    bcop.op = cop->mode;
    bcop.dstBounds = *dstBounds;
    bcop.srcBounds = cop->srcBounds;
    bcop.srcAS = cop->srcAS;
    bcop.dstAS = cop->dstAS;
    bcop.info = cop->info;
    bcop.dissolveFactor = cop->alpha;

    db = (cop->dstCH==VISCHAN)?cop->dst.bag->visbits : cop->dst.bag->backbits;
    bcop.srcType = BM_NOSRC;	/* default src */
    if (cop->src.any) {
	switch (cop->src.any->type) {
	case BAG:
	{
	    bcop.srcType = BM_BITMAPSRC;
	    bcop.src.bm = (cop->srcCH == VISCHAN) ? cop->src.bag->visbits :
		cop->src.bag->backbits;
	    if (bcop.src.bm->isa != remoteBM) {
		/* Make the source public */
		bcop.src.bm = psb = bm_makePublic(bcop.src.bm,
		    &bcop.srcBounds, db->type /* if nonzero */);
	    }
	    break;
	}
	case PATTERN:
	    bcop.srcType = BM_PATTERNSRC;
	    bcop.src.pat = cop->src.pat;
	    break;
	}
    }
    bm_composite(db, &bcop);
    if (psb) bm_delete(psb);
}

void NDDisplayCursor(NXDevice *device, NXCursorInfo *nxci)
{
#ifdef CURSOR_i860
    ps_display_cursor(((ND_PrivInfo *)device->priv)->server_port,
	*nxci->cursorRect);
#endif
#ifdef SERVER_040
    int i, j, width, cursRow, vramRow;
    Bounds bounds, saveRect;
    volatile unsigned int *vramPtr;
    volatile unsigned int *savePtr;
    volatile unsigned int *cursPtr;
    unsigned int s, d, f, rbmask=0xFF00FF00, gamask=0xFF00FF;

    bounds = device->bounds;
    saveRect = *nxci->cursorRect;
    /* Clip saveRect against the screen bounds */
    if (saveRect.miny < bounds.miny) saveRect.miny = bounds.miny;
    if (saveRect.maxy > bounds.maxy) saveRect.maxy = bounds.maxy;
    if (saveRect.minx < bounds.minx) saveRect.minx = bounds.minx;
    if (saveRect.maxx > bounds.maxx) saveRect.maxx = bounds.maxx;
    *nxci->saveRect = saveRect;
    
    vramPtr = (unsigned int *) ((ND_PrivInfo *)device->priv)->fb_addr;
    vramRow = ND_ROWBYTES >> 2;
    vramPtr += (vramRow * (saveRect.miny - bounds.miny)) +
	       (saveRect.minx - bounds.minx);
    width = saveRect.maxx - saveRect.minx;
    vramRow -= width;
    cursPtr = nxci->cursorData32;
    cursPtr += (saveRect.miny - nxci->cursorRect->miny) * 16 +
		saveRect.minx - nxci->cursorRect->minx;
    savePtr = nxci->saveData;
    cursRow = 16 - width;
    
    for (i = saveRect.maxy-saveRect.miny; i>0; i--) {
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



void NDSetSync()
{
    nd_synchronous = PSPopBoolean();
}

void NDCurrentSync()
{
    PSPushBoolean(nd_synchronous);
}

typedef struct _video_bag {
    Bounds windowBounds;	/* global coordinates */
    Bounds bitmapBounds;	/* global coordinates */
    Bounds videoClip;		/* global coordinates */
    Bounds videoBounds;		/* global coordinates of video frame
				   size (640x480 for NTSC) */
    Bounds activeVideo;		/* intersection of above four */
    Point realVideoLoc;		/* user requested video position,
				  in window coords */
    int validBounds;
    int dmaActive;
    RemoteBitmap *rbm;
} VideoBag;
static VideoBag *VBag;

static int CalcActiveVideo(VideoBag *b)
{
    return(sectBounds(&b->windowBounds, &b->bitmapBounds, &b->activeVideo) &&
	   sectBounds(&b->activeVideo, &b->videoClip, &b->activeVideo) &&
	   sectBounds(&b->activeVideo, &b->videoBounds, &b->activeVideo));
}
void SetVideoCallback(NXBag *b, int chan, Bounds bpBounds, Bounds bounds,
		      void *data)
{
    VideoBag *vb = data;
    Bounds theBounds;
    if(chan == VISCHAN) {
	if(sectBounds(&bpBounds, &bounds, &theBounds))
	    ps_video_setrect(vb->rbm->port,
			     theBounds.minx - vb->videoBounds.minx,
			     theBounds.miny - vb->videoBounds.miny,
			     theBounds.maxx - vb->videoBounds.minx,
			     theBounds.maxy - vb->videoBounds.miny);
    }
}
static void SetVideoBounds(VideoBag *vb)
{
    int bx, wx, wy;
    /* convert window-relative video position to bitmap coordinatesd*/
    bx = vb->realVideoLoc.x + vb->windowBounds.minx - vb->bitmapBounds.minx;
    wy = - vb->realVideoLoc.y + vb->windowBounds.maxy;

    /* truncate to the nearest pixel the hardware likes */
    bx = ((bx + 4)&~63) - 4;
    wx = bx + vb->bitmapBounds.minx;
    
    vb->videoBounds.minx = wx;
    vb->videoBounds.maxy = wy;
    vb->videoBounds.maxx = vb->videoBounds.minx + 640;
    vb->videoBounds.miny = vb->videoBounds.maxy - 480;
}

void NDMakeVideoWindow()
{
    int x,y,w;
    NXBag *b;
    NXWindowInfo wi;
    VideoBag *vb;

    w = PSPopInteger();
    y = PSPopInteger();
    x = PSPopInteger();

    if(!(b = NXWID2Bag(w, HackDevice)))
       PSInvalidID();
    if(NXGetWindowInfo(b->layer, &wi))
	PSInvalidID();
       
    VBag = b->priv = vb = calloc(1, sizeof(VideoBag));
    vb->windowBounds = vb->videoClip = wi.bounds;
    vb->bitmapBounds = b->visbits->bounds;
    vb->realVideoLoc.x = x;
    vb->realVideoLoc.y = y;
    SetVideoBounds(vb);
    vb->rbm = (RemoteBitmap *)b->visbits;

    ps_video_clearrect(vb->rbm->port,0,0,640,480);

    if(CalcActiveVideo(vb)) 
	NXRenderInBounds(b->layer, &vb->activeVideo, SetVideoCallback, vb);
    /* now install the hooks so we can keep the mask up to date */
    NXSetHookMask(b, (1 << NX_OBSCUREWINDOW) |
		  ( 1 <<NX_REVEALWINDOW) |
		  ( 1 << NX_MOVEWINDOW));
    
    ps_set_video_loc(vb->rbm->port, vb->rbm->addr,
		     vb->videoBounds.minx - vb->bitmapBounds.minx,
		     vb->videoBounds.miny - vb->bitmapBounds.miny);
}

void NDHook(NXHookData *hd)
{
    Bounds fixBounds;
    /* okay this is our video window */
    switch(hd->type) {
    case NX_OBSCUREWINDOW:
	if(sectBounds(&hd->d.obscure.bounds,&VBag->activeVideo, &fixBounds))
	    ps_video_clearrect(VBag->rbm->port,
			     fixBounds.minx - VBag->videoBounds.minx,
			     fixBounds.miny - VBag->videoBounds.miny,
			     fixBounds.maxx - VBag->videoBounds.minx,
			     fixBounds.maxy - VBag->videoBounds.miny);
	break;
    case NX_REVEALWINDOW:
	if(sectBounds(&hd->d.reveal.bounds,&VBag->activeVideo, &fixBounds))
	    ps_video_setrect(VBag->rbm->port,
			     fixBounds.minx - VBag->videoBounds.minx,
			     fixBounds.miny - VBag->videoBounds.miny,
			     fixBounds.maxx - VBag->videoBounds.minx,
			     fixBounds.maxy - VBag->videoBounds.miny);
	break;
    case NX_MOVEWINDOW:
	switch(hd->d.move.stage) {
	case 0:
	    if( VBag->dmaActive)
		ps_video_dma(VBag->rbm->port, 0);
	    break;
	case 1:
	    /* offset the things that move with the window */
	    OFFSETBOUNDS(VBag->windowBounds,hd->d.move.delta.x,
			 hd->d.move.delta.y);
	    OFFSETBOUNDS(VBag->videoClip,hd->d.move.delta.x,
			 hd->d.move.delta.y);
	    SetVideoBounds(VBag);
	    ps_video_clearrect(VBag->rbm->port,0,0,640,480);
	    ps_set_video_loc(VBag->rbm->port, VBag->rbm->addr,
			     VBag->videoBounds.minx -
			     VBag->bitmapBounds.minx,
			     VBag->videoBounds.miny -
			     VBag->bitmapBounds.miny);
	    if(CalcActiveVideo(VBag)) {
		NXRenderInBounds(hd->bag->layer, &VBag->activeVideo,
				SetVideoCallback, VBag);
		if(VBag->dmaActive)
		    ps_video_dma(VBag->rbm->port, 1);
	    }
	    break;
	}
	break;
	
    }
}
    
    

void NDVideoDma()
{
    int w = PSPopInteger();
    NXBag *b;

    if(!(b = NXWID2Bag(w, HackDevice))) {
	if(b->priv != VBag)
	    PSInvalidID();
    }
    ps_video_dma(VBag->rbm->port, VBag->dmaActive = PSPopBoolean());
}

static void NDMoveVideo()
{
    int w = PSPopInteger();
    NXBag *b;

    if(!(b = NXWID2Bag(w, HackDevice))) {
	if(b->priv != VBag)
	    PSInvalidID();
    }
    VBag->realVideoLoc.y = PSPopInteger();
    VBag->realVideoLoc.x = PSPopInteger();
    ps_video_clearrect(VBag->rbm->port,0,0,640,480);
    SetVideoBounds(VBag);
    ps_set_video_loc(VBag->rbm->port, VBag->rbm->addr,
		     VBag->videoBounds.minx - VBag->bitmapBounds.minx,
		     VBag->videoBounds.miny - VBag->bitmapBounds.miny);
    if(CalcActiveVideo(VBag)) 
	NXRenderInBounds(b->layer, &VBag->activeVideo,
			SetVideoCallback, VBag);

}
static void NDSetVideoClipRect()
{
    int x,y,w,h,win = PSPopInteger();
    NXBag *b;

    if(!(b = NXWID2Bag(win, HackDevice))) {
	if(b->priv != VBag)
	    PSInvalidID();
    }
    h = PSPopInteger();
    w = PSPopInteger();
    y = PSPopInteger();
    x = PSPopInteger();
    VBag->videoClip.minx = x + VBag->windowBounds.minx;
    VBag->videoClip.maxy = VBag->windowBounds.maxy - y;
    VBag->videoClip.maxx = VBag->videoClip.minx + w;
    VBag->videoClip.miny = VBag->videoClip.maxy - h;

	
    ps_video_clearrect(VBag->rbm->port,0,0,640,480);

    if(CalcActiveVideo(VBag)) 
	NXRenderInBounds(b->layer, &VBag->activeVideo,
			SetVideoCallback, VBag);

}

void NDFreeWindow(NXBag *bag)
{
    if(bag->priv) {
	/* stop video, and free structure */
	ps_video_dma(VBag->rbm->port, 0);
	free(VBag);
	VBag = 0;
    }
    if (bag->backbits) bm_delete(bag->backbits);
    
}

void NDIICSendBytes()
{
    int len = PSStringLength();
    int iicaddr;
    unsigned char *iicdata = malloc(len + 1);
    
    PSPopString(iicdata, len + 1);
    iicaddr = *iicdata;
    ps_iicsendbytes(((RemoteBitmap *)HackBM)->port,iicaddr, iicdata + 1,
		    len - 1);
    free(iicdata);
}

void NDInitScreen(NXDevice *device)
{
    int i;
    kern_return_t r;
    port_t owner_port = 0;
    ND_PrivInfo *priv;
    static int registerops = 0;
    static NXRegOpsVector ndops = {
	{"nd_iicsendbytes", NDIICSendBytes},
	{"nd_setsync", NDSetSync},
	{"nd_currentsync", NDCurrentSync},
	{"nd_videodma", NDVideoDma},
	{"nd_makevideowindow", NDMakeVideoWindow},
	{"nd_setvideocliprect", NDSetVideoClipRect},
	{"nd_movevideo", NDMoveVideo},
	{NULL, NULL}
    };
    
    if(!registerops) {
	registerops = 1;
	NXRegisterOps(ndops);
    }
    priv = (ND_PrivInfo *) calloc(1, sizeof(ND_PrivInfo));
#ifdef SERVER_040
    /* Open the board */
    r = ND_Open( nd_dev_port, (int)device->slot );
    if ( r != KERN_SUCCESS )
	mach_error( "ND_BootKernel", r );	/* Shouldn't happen */

    r = ND_GetOwnerPort( nd_dev_port, (int)device->slot, &owner_port );
    if ( r != KERN_SUCCESS )
    	mach_error( "ND_GetOwnerPort", r );	/* Shouldn't happen */

    /* Map the frame buffer into our virtual address space. */
    r = ND_MapFramebuffer( nd_dev_port, owner_port, task_self(),
    			   (int)device->slot, &priv->fb_addr, &priv->fb_size );
    if ( r != KERN_SUCCESS )
    	mach_error( "ND_MapFramebuffer", r );	/* Shouldn't happen */
#endif    
    /* Allocate a reply port for message replies from the ND board. */
    if ( (r = port_allocate( task_self(), &priv->reply_port)) != KERN_SUCCESS)
    	mach_error( "port_allocate", r );	/* Shouldn't happen */
    /* Fork off server process on host */
    if (shouldfork) {
	if (!fork()) {
	    execl("/usr/lib/NextStep/Displays/NextDimension/server",
		"server", 0);
	    printf("FAILURE IN NEXTDIMENSION SERVER\n");
	    exit(1);
	}
    }
#ifdef SERVER_040
    /* Now wait for server process to register its port */
    for (i=0; i<5; i++, sleep(1)) {
	if ((r = netname_look_up(name_server_port, "", "ps_server",
	    &priv->server_port)) == KERN_SUCCESS) break;
    }
#else
    for (i=0; i<5; i++, sleep(1))
	if ((r = ND_Port_look_up(nd_dev_port,(int)device->slot,
	    "ps_server",&priv->server_port)) == KERN_SUCCESS) break;
#endif
    if (r != KERN_SUCCESS) {			/* mustn't happen */
	mach_error( "netname_look_up for ps_server failed", r);
	exit(1);
    }
    ps_init(priv->server_port, nd_dev_port, owner_port, (int)device->slot,
	device->bounds);

    HackDevice  = device;
    HackBM = device->bm = bm_newRemote(NX_TWENTYFOURBITRGB, 1,
	priv->server_port, &device->bounds);

    device->romid = ND_ROMID;
    device->visDepthLimit = NX_TWENTYFOURBITRGB;
    device->priv = (void *)priv;
}

void NDMark(NXBag *bag, int channel, MarkRec *mrec, Bounds *markBds,
    Bounds *bpBds)
{
    Bitmap *bm = (channel == VISCHAN) ? bag->visbits : bag->backbits;
    bm_mark(bm, mrec, markBds, bpBds);
}

void NDMoveWindow(NXBag *bag, short dx, short dy, Bounds *old, Bounds *new)
{
    if (bag->backbits) bm_offset(bag->backbits, dx, dy);
}

void NDNewAlpha(NXBag *bag)
{
    if (bag->backbits) bm_newAlpha(bag->backbits, true);
}

void NDNewWindow(NXBag *bag, Bounds *bounds, int windowType, int depth,
    		 int local)
{
    bag->visbits = bag->device->bm;
    if (windowType == NONRETAINED) return;
    depth = (depth<NX_OTHERBMTYPE)?NX_OTHERBMTYPE:(depth>NX_TWENTYFOURBITRGB)?
	NX_TWENTYFOURBITRGB:depth;
    /* If screen depth doesn't match initial depth, then set mismatch flag.
     * NOTE: This is only necessary for RETAINED windows */
    if (depth != NX_TWENTYFOURBITRGB)
	bag->mismatch = 1;
    bag->backbits = bm_newRemote(depth, 0, ((ND_PrivInfo *)
	bag->device->priv)->server_port, bounds);
}

void NDPromoteWindow(NXBag *bag, Bounds *bounds, int newDepth, int windowType,
		     DevPoint phase)
{
    BMClass *class;
    Bitmap *bm, *oldbm;
    port_t port;
    
    port = ((ND_PrivInfo *)bag->device->priv)->server_port;
    oldbm = bag->backbits;
    if (windowType == NONRETAINED || newDepth == oldbm->type) return;
    bm = bm_newRemote(newDepth, 0, port, bounds);
    ps_promoteWindow(port, ((RemoteBitmap *)bm)->addr,
        ((RemoteBitmap *)oldbm)->addr, *bounds, phase, newDepth);
	
    bm_delete(oldbm);
    bag->backbits = bm;
    bag->mismatch = (bm->type != NX_TWENTYFOURBITRGB);
}

void NDRegisterScreen(NXDevice *d)
{
    kern_return_t r;

    r = ND_RegisterThyself(nd_dev_port, (int)d->slot, (int)d, d->bounds.minx,
	d->bounds.maxx, d->bounds.miny, d->bounds.maxy);
}

void NDRemoveCursor(NXDevice *device, NXCursorInfo *nxci)
{
#ifdef CURSOR_i860
    ps_remove_cursor(((ND_PrivInfo *)device->priv)->server_port);
#endif
#ifdef SERVER_040
    Bounds saveRect;
    short i, j, width, vramRow;
    volatile unsigned int *vramPtr;
    volatile unsigned int *savePtr;
    
    saveRect = *nxci->saveRect;
    savePtr = nxci->saveData;
    vramPtr = (unsigned int *) ((ND_PrivInfo *)device->priv)->fb_addr;
    vramRow = ND_ROWBYTES >> 2;
    vramPtr += (vramRow * (saveRect.miny - device->bounds.miny)) +
	       (saveRect.minx - device->bounds.minx);
    width = saveRect.maxx - saveRect.minx;
    vramRow -= width;
    for (i = saveRect.maxy - saveRect.miny; --i>=0; ) {
	for (j = width; --j>=0; ) *vramPtr++ = *savePtr++;
	vramPtr += vramRow;
    }
#endif
}

void NDSetCursor(NXDevice *device, NXCursorInfo *nxci, LocalBitmap *lbm)
{
#ifdef CURSOR_i860
    ps_set_cursor(((ND_PrivInfo *)device->priv)->server_port, lbm->base.type,
	lbm->rowBytes, lbm->bits, lbm->byteSize, lbm->abits, lbm->abits ?
	lbm->byteSize : 0);
#endif
#ifdef SERVER_040
    volatile unsigned int *cp, *data;
    short i, j, rowLongs;
    
    if (nxci->set.curs.state32) return;
    nxci->set.curs.state32 = 1;
    data = lbm->bits;
    rowLongs = lbm->rowBytes >> 2;
    cp = nxci->cursorData32;
    
    switch(lbm->base.type) {
    case NX_TWOBITGRAY: {
	unsigned int d, m, e, f;
	volatile unsigned int *mask;
	
	mask = lbm->abits;
	for (i=16; --i>=0; ) {
	    m = *mask; mask += rowLongs;	/* Get mask data and inc ptr */
	    d = *data; data += rowLongs;	/* Get grey data and convert */
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
	break;
    }
    case NX_TWENTYFOURBITRGB:
	for (i=16; --i>=0; ) {
		*cp++ = *data++;
		*cp++ = *data++;
		*cp++ = *data++;
		*cp++ = *data++;
		*cp++ = *data++;
		*cp++ = *data++;
		*cp++ = *data++;
		*cp++ = *data++;
		*cp++ = *data++;
		*cp++ = *data++;
		*cp++ = *data++;
		*cp++ = *data++;
		*cp++ = *data++;
		*cp++ = *data++;
		*cp++ = *data++;
		*cp++ = *data++;
	}
	break;
    }
#endif
}

void NDSyncCursor(NXDevice *device, int sync)
{
    port_t port;
    port = ((ND_PrivInfo *)device->priv)->server_port;
    if (sync)
	ps_sync_cursor(port);
    else
	ps_sync_cursor_a(port);
}


int NDWindowSize(NXBag *bag)
{
    int l, r, size;
    size = sizeof(ND_PrivInfo);
    if (bag->backbits) size += bm_sizeInfo(bag->backbits, &l, &r);
    return size;
}

void NDPing(NXDevice *device)
{
    port_t port;
    port = ((ND_PrivInfo *)device->priv)->server_port;
    ps_ping(port);
}

void MsgError(kern_return_t errcode)
{
    os_printf("nd msg error %d\n", errcode);
}

