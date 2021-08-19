/*****************************************************************************

    remote.c
    Abstract superclass for all remotely stored bitmaps
    
    CONFIDENTIAL
    Copyright 1990 NeXT Computer, Inc. as an unpublished work.
    All Rights Reserved.

    01Aug90 Ted Cohn
    
    Modifications:

******************************************************************************/

#import "package_specs.h"
#import <mach.h>
#import <sys/message.h>
#import "bitmap.h"
#import "remote.h"
#import "both.h"

extern port_t mig_get_reply_port();
static BMClass *bmClassFromDepth[] = {mp12, mp12, bm38, bm34, bm38};


/* all mig generaterd message sent by remote bitmaps go through this wrapper
    */
int nd_synchronous = 0;
    
msg_return_t
ps_msg_send(msg_header_t *m, msg_option_t option, msg_timeout_t timeout)
{
	msg_return_t msg_result = msg_send(m, option, timeout);
	if (msg_result != SEND_SUCCESS) 
		return msg_result;	
	/* ping if desired and no reply is expected */	
	if(nd_synchronous && !m->msg_local_port) ps_ping(m->msg_remote_port);	
	return(msg_result);
}
static Bitmap *RBMNewRemote(int type, int special, port_t port, Bounds *b)
{
    RemoteBitmap *rbm;

    rbm = (RemoteBitmap *)(*bmClass->new)(remoteBM, b, false);
    rbm->port = port;
    ps_rbm_newRemote(port, type, special, *b, &rbm->addr, &rbm->base);
    rbm->base.isa = remoteBM;
    return (Bitmap *)rbm;
}

static Bitmap *RBMRemoteFromData(int type, port_t port, Bounds *b,
    unsigned int *bits, unsigned int *abits, int byteSize, int rowBytes)
{
#if 0
    RemoteBitmap *rbm = (RemoteBitmap *)(*bmClass->new)(remoteBM, b, false);
    ps_rbm_newFromData(port, type, *b, bits, byteSize, abits, abits ? byteSize
	: 0, rowBytes, &rbm->addr, &rbm->base);
    rbm->base.isa = remoteBM;
    rbm->port = port;
    return (Bitmap *)rbm;
#endif
}

static void RBMFree(Bitmap *bm)
{
    RemoteBitmap *rbm = (RemoteBitmap *)bm;
    ps_rbm_free(rbm->port, rbm->addr);
    (*bmClass->_free)((Bitmap *)bm);	/* super free */
}

static void RBMNewAlpha(Bitmap *bm, int initialize)
{
    RemoteBitmap *rbm = (RemoteBitmap *)bm;
    if (rbm->base.alpha || rbm->base.vram) return;
    rbm->base.alpha = 1;
    ps_rbm_newAlpha(rbm->port, rbm->addr, initialize);
}

static void RBMComposite(Bitmap *bm, BMCompOp *bcop)
{
    RemoteBitmap *rbm = (RemoteBitmap *)bm;
    switch(bcop->srcType) {
    case BM_NOSRC:
        ps_rbm_composite(rbm->port, rbm->addr, *bcop);
	break;
    case BM_BITMAPSRC:
    {
        if (bcop->src.bm->isa == remoteBM) {
	    ps_rbm_composite_bm(rbm->port, rbm->addr,
		((RemoteBitmap *)bcop->src.bm)->addr, *bcop);
	} else {
	    LocalBitmap *lbm = (LocalBitmap *)bcop->src.bm;
	    ps_rbm_composite_lbm(rbm->port, rbm->addr, *bcop, *lbm, lbm->bits,
		lbm->byteSize, lbm->abits, lbm->abits ? lbm->byteSize : 0);
	}
	break;
    }
    case BM_PATTERNSRC:
	ps_rbm_composite_pat(rbm->port, rbm->addr, *bcop, *bcop->src.pat);
	break;
    }
}

static void RBMConvertFrom(Bitmap *db, Bitmap *sb, Bounds *dBounds,
			   Bounds *sBounds, DevPoint phase)
{
    RemoteBitmap *rbm = (RemoteBitmap *)db;

    if (sb->isa == remoteBM) {	/* both are remote bitmaps */
	ps_rbm_convertFrom(rbm->port, rbm->addr, *dBounds, *sBounds, phase,
	    ((RemoteBitmap *)sb)->addr);
    } else {			/* only dst is remote, src is local */
        LocalBitmap *lbm = (LocalBitmap *)sb;
	ps_rbm_convertFrom_local(rbm->port, rbm->addr, *dBounds, *sBounds,
	    phase, *lbm, lbm->bits, lbm->byteSize, lbm->abits, lbm->abits ?
	    lbm->byteSize : 0);
    };
}

static Bitmap *RBMMakePublic(Bitmap *bm, Bounds *hintBounds, int hintDepth)
{
    static MakePublicReq mreq = {
	{ 0, false, sizeof(MakePublicReq), 0, 0, 0, PSMAKEPUBLICID },
	{ MSG_TYPE_INTEGER_32, 32, 4, true, 0, 0, 0 },
	0, 0, {0,0,0,0}
    };
    Bitmap *lbm;
    kern_return_t r;
    MakePublicReply mreply;

    /* Bundle up args in request message */
    mreq.h.msg_remote_port = ((RemoteBitmap *)bm)->port;
    mreq.h.msg_local_port = mig_get_reply_port();
    mreq.rbm = ((RemoteBitmap *)bm)->addr;
    mreq.hintBounds = *hintBounds;
    mreq.hintDepth = hintDepth;

    /* Set up buffer for reply message */
    bzero(&mreply, sizeof(MakePublicReply));
    mreply.h.msg_local_port = mreq.h.msg_local_port;
    mreply.h.msg_size = sizeof(MakePublicReply);
    
    r = msg_send( (msg_header_t *) &mreq, MSG_OPTION_NONE, 0 );
    if (r != RCV_SUCCESS)
	/* shouldn't happen */
	os_printf("makePublic: msg_send returned %D\n", r);
    r = msg_receive( (msg_header_t *) &mreply, MSG_OPTION_NONE, 0 );
    if (r != RCV_SUCCESS) {
	/* shouldn't happen */
	if (r == RCV_INVALID_PORT) mig_dealloc_reply_port();
	os_printf("makePublic: msg_receive returned %D\n", r);
    }
    lbm = bm_newFromData(bmClassFromDepth[mreply.type], &mreply.bounds,
	mreply.bits, (mreply.oobAbits.msg_type_long_number) ? mreply.abits : 0,
	mreply.oobBits.msg_type_long_number,
	mreply.rowBytes, false, LBM_VMDEALLOCATE);
    lbm->type = mreply.type;
    return lbm;
}

/* needs to bundle up mrec before ps_rbm_mark call */

/* eventually, we don't want this to block ! */
static void mark_msg_send(ArgStreamMsg *m)
{
    kern_return_t r;
    r = msg_send((msg_header_t *)m, MSG_OPTION_NONE, 0);
    if (r != RCV_SUCCESS)
	os_printf("mark_msg_send: msg_send returned %D\n", r);
}

static void RBMMark(Bitmap *bm, MarkRec *mrec, Bounds *markBds, Bounds *bpBds)
{
    RemoteBitmap *rbm = (RemoteBitmap *)bm;
    WriteArgStream *as;
    msg_header_t h;
    MarkRequest mreq;
    
    /*  to send a mark call we use the argument stream abstraction, first
	we create a msg_header_t that can be used as a prototype for
	sending args */
    h.msg_remote_port = ((RemoteBitmap *)bm)->port;
    h.msg_local_port = 0; /* no reply expected */
    h.msg_id = PSMARKID;
    
    as = as_newWrite(&h, mark_msg_send);
    /* now wad up the arguments */
    mreq.bm_addr = rbm->addr;
    mreq.mr = *mrec;
    mreq.markBds = *markBds;
    mreq.bpBds = *bpBds;
    as_write_C_type(as, &mreq, MarkRequest);
    packDevPrim(as, mrec->graphic);
    packDevPrim(as, mrec->clip);
    packDevMarkInfo(as, &mrec->info);
    as_flushmsg(as);
    as_destroyWrite(as);
    if(nd_synchronous) ps_ping(((RemoteBitmap *)bm)->port);
}


static void RBMOffset(Bitmap *bm, short dx, short dy)
{
    RemoteBitmap *rbm = (RemoteBitmap *)bm;
    OFFSETBOUNDS(rbm->base.bounds, dx, dy);
    ps_rbm_offset(rbm->port, rbm->addr, dx, dy);
}

static int RBMSizeInfo(Bitmap *bm, int *localSize, int *remoteSize)
{
    RemoteBitmap *rbm = (RemoteBitmap *)bm;
    *remoteSize = ps_rbm_sizeInfo(rbm->port, rbm->addr);
    *localSize = rbm->base.isa->_size;
    return *remoteSize + *localSize;
}

static void RBMInitClassVars(BMClass *class)
{
    RemoteBMClass *myclass = (RemoteBMClass *) class;

    /* initialize superclass */
    (*bmClass->_initClassVars)(class);

    /* now selectively override superclasses' functions where needed */
    class->_free = RBMFree;
    class->_size = sizeof(RemoteBitmap);
    class->makePublic = RBMMakePublic;
    class->mark = RBMMark;
    class->newAlpha = RBMNewAlpha;
    class->offset = RBMOffset;
    class->sizeInfo = RBMSizeInfo;
    
    /* now initialize our class by copying from the template below */
    bcopy((char *)remoteBM + sizeof(BMClass), (char *)class + sizeof(BMClass),
	  sizeof(RemoteBMClass) - sizeof(BMClass));
}

/* this structure is filled out for use by subclasses needing a `super'
   method.  This structure is actually never used as the isa pointer of
   a bitmap (it's an abstract superclass)
   */
RemoteBMClass _remoteBM = {
    { 
	0,			/* private initialized boolean */
	RBMInitClassVars,	/* subclass initialization method */
	0,			/* don't call new method (call newRemote) */
	RBMFree,		/* _free method */
	sizeof(RemoteBitmap),	/* _size */
	RBMComposite,
	RBMConvertFrom,
	RBMMakePublic,
	RBMMark,
	RBMNewAlpha,
	RBMOffset,
	0,			/* sizeBits never called from client */
	RBMSizeInfo,
    },
    RBMNewRemote,		/* Only way to create remote bitmaps */
    RBMRemoteFromData		/* Creates remote bitmap from local host bm */
};

