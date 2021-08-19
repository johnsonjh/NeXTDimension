/*****************************************************************************

    process.c
    Service client requests on NextDimension
    
    CONFIDENTIAL
    Copyright 1990 NeXT Computer, Inc. as an unpublished work.
    All Rights Reserved.

    01Aug90 Ted Cohn
    
    Modifications:

******************************************************************************/

#import <stdlib.h>
#import <mach.h>
#import <sys/message.h>
#import <ND/NDlib.h>
#import "package_specs.h"
#import "bitmap.h"
#import "server.h"
#import "both.h"
#import "mp12.h"
#import "framedev.h"
#import "ND/NDGlobals.h"

Bounds fb_rect;
int fb_size;
unsigned int *fb_addr;
static BMClass *bmClassFromDepth[] = {mp12, mp12, bm38, bm34, bm38};
extern int nd_flushCEQ;

#define VMFREE(ptr,size) \
    if (size) vm_deallocate(task_self(), (vm_address_t)ptr, (vm_size_t)size)


void ps_ping ( port_t port )
{
    FlushCEQ();
}


void ps_rbm_composite( port_t port, int dbm, BMCompOp bcop )
{
    bcop.info.halftone = NULL;
    bm_composite((Bitmap *)dbm, &bcop);
}

static inline int bm_bogus(Bitmap *bm)
{
    if(bm->isa == mp12) return 0;
    if(bm->isa == bm34) return 0;
    if(bm->isa == bm38) return 0;
    return 1;
}

void ps_rbm_composite_bm( port_t port, int db, int sb, BMCompOp bcop )
{
    Bitmap *psb = NULL;
    Bitmap *csb = NULL;
    Bitmap *dbm = (Bitmap *)db;
    Bitmap *sbm = (Bitmap *)sb;
    bcop.src.bm = sbm;
    bcop.info.halftone = NULL;
    if(bm_bogus(sbm)){
	    printf("Bogus bitmap->isa, address of Bitmap is %X\n",sb);
	    os_raise(0,"ps_rbm_composite: Bogus bitmap!!!\n");
	}
    if (dbm->isa != sbm->isa) {
	/* HACK: Allow mp12 bitmaps to convert directly to bm38 bitmaps */
	if (sbm->isa == mp12 && dbm->isa == bm38)
	    psb = bm_dup(sbm);
	else {
	    psb = bm_makePublic(sbm, &bcop.srcBounds, dbm->type);
	}
	
	if (bcop.op == COPY) {
	    bm_convertFrom(dbm, psb, &bcop.dstBounds,
		&bcop.srcBounds, bcop.info.screenphase);
	    goto Cleanup;
	} else {
	    csb = bm_new(dbm->isa, &bcop.srcBounds, false);
	    bm_convertFrom(csb, psb, &csb->bounds, &csb->bounds,
		bcop.info.screenphase);
	    bcop.src.bm = csb;
	}
    }
    bm_composite(dbm, &bcop);
Cleanup:
    if (psb) bm_delete(psb);
    if (csb) bm_delete(csb);
}


void ps_rbm_composite_lbm( port_t port, int db, BMCompOp bcop,
			   LocalBitmap sb, pointer_t bits,
			   unsigned int bitsCnt, pointer_t abits,
			   unsigned int abitsCnt )
{
    Bitmap *csb = NULL;
    Bitmap *dbm;
    Bitmap *sbm;
    dbm = (Bitmap *)db;
    bcop.src.bm = sbm = (Bitmap *)&sb;
    sb.base.isa = bmClassFromDepth[sbm->type];
    sb.bits = (unsigned int *)bits;
    sb.abits = (unsigned int *)abits;
    if (sbm->isa != dbm->isa || sbm->type != dbm->type) {
	if (bcop.op == COPY) {
	    bm_convertFrom(dbm, sbm, &bcop.dstBounds,
		&bcop.srcBounds, bcop.info.screenphase);
	    goto Cleanup;
	} else {
	    csb = bm_new(dbm->isa, &bcop.srcBounds, false);
	    bm_convertFrom(csb, sbm, &csb->bounds, &csb->bounds,
		bcop.info.screenphase);
	    bcop.src.bm = csb;
	}
    }
    bcop.info.halftone = NULL;
    bm_composite(dbm, &bcop);
Cleanup:
    if (csb) bm_delete(csb);
    VMFREE(bits,bitsCnt);
    VMFREE(abits,abitsCnt);
}


void ps_rbm_composite_pat( port_t port, int dbm, BMCompOp bcop, Pattern pat )
{
    bcop.src.pat = (Pattern *)&pat;
    bcop.info.halftone = NULL;
    pat.halftone = NULL;
    bm_composite((Bitmap *)dbm, &bcop);
}


void ps_rbm_convertFrom( port_t port, int dbm, Bounds dBounds, Bounds sBounds,
			 DevPoint phase, int sbm )
{
    bm_convertFrom((Bitmap *)dbm, (Bitmap *)sbm, &dBounds, &sBounds, phase);
}


void ps_rbm_convertFrom_local( port_t port, int dbm, Bounds dBounds,
			       Bounds sBounds, DevPoint phase, LocalBitmap sbm,
			       pointer_t bits, unsigned int bitsCnt,
			       pointer_t abits, unsigned int abitsCnt )
{
    sbm.bits = (unsigned int *)bits;
    sbm.abits = (unsigned int *)abits;
    bm_convertFrom((Bitmap *)dbm, (Bitmap *)&sbm, &dBounds, &sBounds, phase);
    VMFREE(bits, bitsCnt);
    VMFREE(abits, abitsCnt);
}


void ps_init( port_t port, port_t ndport, port_t owner, int slot,
	      Bounds bounds )
{
    kern_return_t r;

    fb_rect = bounds;
    /* Map the frame buffer into our virtual address space. */
#if defined(i860)
    fb_size = PADDR_FRAMESTORE_SIZE;
    r = vm_map_hardware(task_self(),&fb_addr, ADDR_FRAMESTORE, fb_size, TRUE);
    vm_cacheable( task_self(), fb_addr, fb_size, TRUE );
#else
    r = ND_MapFramebuffer( ndport, owner, task_self(),
    			   slot, &fb_addr, &fb_size );
#endif
    if ( r != KERN_SUCCESS )
    	mach_error( "ND_MapFramebuffer", r );	/* Shouldn't happen */
    fb_addr += 4;
}

unsigned int death_isa = 0;
void ps_rbm_newRemote( port_t port, int rbmtype, int special, Bounds bounds,
		       int *addr, Bitmap *base )
{
    Bitmap *bm;
    if (special) {
	/* `+4' hack to cover hardware vram offset bug */
	bm = bm_newFromData(bm38, &bounds, fb_addr + 4, NULL, fb_size, ND_ROWBYTES,
			    true, LBM_DONTFREE);
    } else {
	bm = bm_new(bmClassFromDepth[rbmtype], &bounds, false);
    }
    *addr = (int)bm;
    *base = *bm;
}


void ps_rbm_remoteFromData( port_t port, int rbmtype, Bounds bounds,
    pointer_t bits, unsigned int bitsCnt, pointer_t abits, unsigned int
    abitsCnt, int rowBytes, int *addr, Bitmap *base )
{
    Bitmap *bm;
    bm = bm_newFromData(bmClassFromDepth[rbmtype], &bounds,
	(unsigned int *)bits, (unsigned int *)abits, bitsCnt, rowBytes,
	false, LBM_VMDEALLOCATE);
    *addr = (int)bm;
    *base = *bm;
}


void ps_rbm_newAlpha( port_t port, int rbm, boolean_t init )
{
    bm_newAlpha((Bitmap *)rbm, init);
}


void ps_rbm_free( port_t port, int rbm )
{
    bm_free((Bitmap *)rbm);
}


static MakePublicReply mreply = {
    { 0, false, sizeof(MakePublicReply), 0, 0, 0, PSMAKEPUBLICID+100 },
    { MSG_TYPE_INTEGER_32, 32, 4, 1, 0, 0, 0 }, 0, 0, {0,0,0,0},
    { { 0, 0, 0, 0, 1, 0, 0 }, MSG_TYPE_CHAR, 8, 0 }, 0,
    { { 0, 0, 0, 0, 1, 0, 0 }, MSG_TYPE_CHAR, 8, 0 }, 0
};


void ps_rbm_makePublic( MakePublicReq *mreq )
{
    Bitmap *bm;
    LocalBitmap *lbm;

    bm = bm_makePublic((Bitmap *)mreq->rbm,&mreq->hintBounds,mreq->hintDepth);
    lbm = (LocalBitmap *)bm;
    mreply.type = bm->type;
    mreply.bounds = bm->bounds;
    mreply.rowBytes = lbm->rowBytes;
    mreply.bits = (char *)lbm->bits;
    mreply.oobBits.msg_type_long_number = lbm->byteSize;
    mreply.h.msg_size = sizeof(MakePublicReply);
    mreply.h.msg_local_port = PORT_NULL;
    mreply.h.msg_remote_port = mreq->h.msg_remote_port;
    if (lbm->abits) {
        mreply.abits = (char *)lbm->abits;
	mreply.oobAbits.msg_type_long_number = lbm->byteSize;
    } else { /* decrease the size of the message */
        mreply.abits = (char *)0;
	mreply.oobAbits.msg_type_long_number = 0;
	mreply.h.msg_size -= sizeof(char *) + sizeof(msg_type_long_t);
    }
    sendreply((msg_header_t *)&mreply);
    bm_delete(bm);
}


void ps_rbm_offset( port_t port, int rbm, short dx, short dy )
{
    offsetBounds(&((Bitmap *)rbm)->bounds, dx, dy);
}

/* used by mark argument stream to receive messages */
static void mark_receive_msg(ArgStreamMsg *m)
{
    msg_receive((msg_header_t *)m, MSG_OPTION_NONE, 0);
}

/* cleanup function called in top level error handler.  allows us
   a chance to cleanup if an exception was raised 
   we use a static handle for the arg stream so that it can be cleaned up
   if it was left open by a prevous call (because an error was raised).
   This allows us to robustly cleanup the arg stream without needed to
   do a DURING every mark
   */
static ReadArgStream *AS = 0;

void MarkExceptionCleanup()
{
    /* cleanup AS if it was left around */
    if(AS) {
	printf("open argstream discovered, cleaning up");
	as_destroyRead(AS);
	AS = 0;
    }
}
    

void ps_rbm_mark(ArgStreamMsg *m)
{
    MarkRequest *mreq;
    DevMarkInfo *mi;
    static Device *ps_device = 0;

    /* create a frame device */
    if(!ps_device) {
	/* we only need a skeletal device, the only thing mark will
	   do is pull procedure vectors out of it */
	ps_device =  calloc(1, sizeof(Device));
	ps_device->procs = fmProcs;
    }

    /* create argument stream for reading */
    AS = as_newRead(m, mark_receive_msg);
    mreq = as_read(AS, sizeof(MarkRequest));
    mreq->mr.graphic = unpackDevPrim(AS, mreq->mr.graphic);
    mreq->mr.clip = unpackDevPrim(AS, mreq->mr.clip);
    mi = unpackDevMarkInfo(AS);
    mreq->mr.info = *mi;
    mreq->mr.device = ps_device; 
    bm_mark((Bitmap *)mreq->bm_addr, &mreq->mr, &mreq->markBds, &mreq->bpBds);
    as_destroyRead(AS);
    AS = 0;
}


int ps_rbm_sizeInfo( port_t port, int rbm )
{
    int l, r;
    return bm_sizeInfo((Bitmap *)rbm, &l, &r);
}

void ps_promoteWindow(port_t port, int new, int old, Bounds bounds,
    DevPoint phase, int newDepth)
{
    Bitmap *pub;
    pub = bm_makePublic((Bitmap *)old, &bounds, newDepth);
    bm_convertFrom((Bitmap *)new, pub, &bounds, &bounds, phase);
    bm_delete(pub);
}


#ifdef CURSOR_i860

void ps_display_cursor( port_t port, Bounds crsrRect )
{
    FlushCEQ();
    nd_flushCEQ++;
    DisplayCursor(&crsrRect);
    nd_flushCEQ--;
}


void ps_remove_cursor( port_t port )
{
    FlushCEQ();
    nd_flushCEQ++;
    RemoveCursor();
    nd_flushCEQ--;
}

void ps_set_cursor( port_t port, int ltype, int rowBytes, pointer_t bits,
		    unsigned int bitsCnt, pointer_t abits,
		    unsigned int abitsCnt )
{
    short i, j, rowLongs;
    volatile unsigned int *cp, *data;
    extern unsigned int nd_cursorData[];

    FlushCEQ();
    data = (unsigned int *)bits;
    rowLongs = rowBytes >> 2;
    cp = nd_cursorData;	/* fill in global cursor image directly */
    
    switch(ltype) {
    case NX_TWOBITGRAY: {
	unsigned int d, m, e, f;
	volatile unsigned int *mask;
	
	mask = (unsigned int *)abits;
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
    VMFREE(bits, bitsCnt);
    VMFREE(abits, abitsCnt);
}

void ps_sync_cursor( port_t port )
{
    FlushCEQ();
}

void ps_sync_cursor_a( port_t port )
{
    FlushCEQ();
}

#endif


void ps_set_video_loc(port_t port, int bm, int x, int y)
{
    LocalBitmap *b = (LocalBitmap *)bm;
    char *dma_addr;

    if(b->base.isa != bm38) {
	printf("Must be a 32-bit bitmap for dma!\n");
	return;
    }
    printf("ps_video_start virt=%X, phys = %X\n", b->bits, kvtophys(b->bits));
    dma_addr = (char *)b->bits + (y-b->base.bounds.miny)*(b->rowBytes)
	+ ((x - b->base.bounds.minx) << 2);
    video_dma_addr((void *)dma_addr, b->rowBytes);
}

void ps_video_dma(port_t port,int on)
{
    if(on) video_dma_start();
    else video_dma_stop();
}

void ps_video_setrect(port_t port, int x, int y, int w, int h)
{

    video_mask_setrect(x,y,w,h);
}
void ps_video_clearrect(port_t port, int x, int y, int w, int h)
{

    video_mask_clearrect(x,y,w,h);
}

/****************************/

