/*****************************************************************************

    markpack.c
    Pack and unpack PostScript Mark streams

    
    CONFIDENTIAL
    Copyright 1990 NeXT Computer, Inc. as an unpublished work.
    All Rights Reserved.

    25Aug90 Peter Graffagnino

    Modified:

******************************************************************************/

#import "package_specs.h"
#import "argstream.h"
#import "bintree.h"

#define LongWordPad(n) (((n) + 3)&~3)

#define TRANSLATION_HACKS
#ifdef TRANSLATION_HACKS

/* since alignment is stricter on the 860 side of things, we hack up the
   sending side to send only structures that are realizeable on the 860.
   The structures affected are DevRun, DevImageInfo, and DevImage.
   */

typedef struct 
    {
    Mtx *mtx;			/* maps from device space to image space */
    DevBounds sourcebounds;	/* source rectangle within source pixel
				 * buffer; given in pixels, not bits */
    DevPoint sourceorigin;	/* image space coords of minx, miny corner
                                 * of source data rectangle */
    DevTrap *trap;		/* pointer to first in array of traps */
    DevShort trapcnt;		/* number of trapezoids */
    DevShort pad;		/* keep it aligned! */
} ExportableDevImageInfo;

typedef struct {
    ExportableDevImageInfo info;	/* DevImageInfo needs to be aligned */
    boolean imagemask:8;
    boolean invert:8;
    DevShort unused;
    DevTfrFcn *transfer;
    DevImageSource *source;
    DevGamutTransfer gt;
    DevRendering r;
} ExportableDevImage;

typedef struct {
    DevBounds bounds;
    DevShort datalen;
    DevShort pad;
    DevShort *data;
    DevShort *indx;
} ExportableDevRun;
    
#endif

/* HACK!!!!!! these values are gripped from device/qintersect.c */
#define INDXSHFT (5)
#define INDXMASK ((1 << INDXSHFT)-1)

#define RunIndexSize(r) (((r->bounds.y.g - r->bounds.y.l + INDXMASK) >> INDXSHFT)*sizeof(DevShort))

#ifdef TRANSLATION_HACKS
static inline void
packRuns(WriteArgStream *as, int n, DevRun *devrun)
{
    ExportableDevRun r[n], *run;
    int i;
    for(i = 0, run = r; i < n; i++, run++, devrun++) {
	run->bounds = devrun->bounds;
	run->datalen = devrun->datalen;
	run->data = devrun->data;
	run->indx = devrun->indx;
	/* write structs one at a time to ensure that they go inline,
	   (since they're on the stack!)
	   */
	as_write_C_type(as, run, ExportableDevRun);
    }
    run = r;

#else
static inline void
packRuns(WriteArgStream *as, int n, DevRun *devrun)
{
  as_write(as, run, n*sizeof(DevRun));
#endif    

    for(; --n >=0; run++) {
	/*enforce padding to longword */
	as_write(as, run->data, LongWordPad(run->datalen*
					       sizeof(DevShort)));
	if(run->indx)
	    as_write(as, run->indx, LongWordPad(RunIndexSize(run)));
    }
}
/* masks are longword aligned per scanline unless mr->unused
   is set, in which case scanlines are byte aligned (but total
   size is still rounded up to longword boundary)  */
#define MaskByteSize(mr) ((mr->unused) \
			   ? (((mr->width+7)>>3)*mr->height + 3) & ~3 \
			   : ((mr->width + 31)>>5)*4*mr->height)

static inline void
packMasks(WriteArgStream *as, int n, DevMask *mask)
{
    as_write(as, mask, n*sizeof(DevMask));
    for(; --n >= 0; mask++) {
	MaskRec *mr = mask->mask;
	int maskbytes;
	as_write_C_type(as, mr, MaskRec);
	maskbytes = MaskByteSize(mr);
	as_write(as, mr->data, maskbytes);
	
    }
}
#ifdef TRANSLATION_HACKS
static inline void
packImage(WriteArgStream *as, int n, DevImage *devimage)
{
    DevTfrFcn *tfr;
    DevImageSource *is;
    ExportableDevImage imagearray[n], *image;
    int i;

    for(i=0, image = imagearray; i < n; i++, image++,devimage++) {
	image->info.mtx = devimage->info.mtx;
	image->info.sourcebounds = devimage->info.sourcebounds;
	image->info.sourceorigin = devimage->info.sourceorigin;
	image->info.trap = devimage->info.trap;
	image->info.trapcnt = devimage->info.trapcnt;
	image->imagemask = devimage->imagemask;
	image->invert = devimage->invert;
	image->unused = devimage->unused;
	image->transfer = devimage->transfer;
	image->source = devimage->source;
	image->gt = devimage->gt;
	image->r = devimage->r;
	as_write_C_type(as, image, ExportableDevImage);
    }
    image = imagearray;
#else
static inline void
packImage(WriteArgStream *as, int n, DevImage *image)
{
    DevTfrFcn *tfr;
    DevImageSource *is;
    as_write(as, image, n*sizeof(DevImage));
#endif    
    
    for(; --n >= 0; image++) {
	as_write_C_type(as, image->info.mtx, Mtx);
	as_write(as, image->info.trap,
		       image->info.trapcnt*sizeof(DevTrap));
	if(tfr = image->transfer) {
	    as_write_C_type(as,tfr, DevTfrFcn);
	    /* we don't write the priv field of the tfr function */
	    if(tfr->white) 	as_write(as, tfr->white, 256);
	    if(tfr->red) 	as_write(as, tfr->red, 256);
	    if(tfr->green)	as_write(as, tfr->green, 256);
	    if(tfr->blue)	as_write(as, tfr->blue, 256);
	    if(tfr->ucr)	as_write(as, tfr->ucr, 256*sizeof(DevShort));
	    if(tfr->bg)	as_write(as, tfr->bg, 256);
	}
	is = image->source;
	as_write_C_type(as, is, DevImageSource);
	as_write(as, is->samples, 5*sizeof(unsigned char *));
	
	if(is->interleaved) 
	    as_write(as, is->samples[0], LongWordPad(is->height*is->wbytes));
	else {
	    /* non-interleaved */
	    int i, nbytes = LongWordPad(is->height*is->wbytes);
	    for(i = 0; i < is->nComponents; i++) 
		as_write(as, is->samples[i], nbytes);
	    if(image->unused)
		/* has alpha component as well */
		as_write(as, is->samples[i], nbytes);
	}

	if(is->decode)
	    as_write(as, is->decode,
			   is->nComponents*sizeof(DevImSampleDecode));
	
	/* ignored fields, don't appear to be used in our code:
	   In DevImage:
	   	DevGamutTransfer gt;
		DevRendering r;
	   In DevImageSource:
		procedure (*sampleProc)();
		char *procData;
	   In DevTfrFcn:
	   	DevPrivate *priv;
	*/
	   
    }
}

void
packDevPrim(WriteArgStream *as, DevPrim *graphic)
{
    for(; graphic; graphic = graphic->next) {
	as_write_C_type(as, graphic, DevPrim);
	switch(graphic->type) {
	case trapType:
	    as_write(as, graphic->value.trap,
			   graphic->items*sizeof(DevTrap));
	    break;
	case runType:
	    packRuns(as, graphic->items, graphic->value.run);
	    break;
	case maskType:
	    packMasks(as, graphic->items, graphic->value.mask);
	    break;
	case imageType:
	    packImage(as, graphic->items, graphic->value.image);
	    break;
	case noneType:
	    break;
	}
    }
}


void
packDevMarkInfo(WriteArgStream *as, DevMarkInfo *info)
{
    NextGSExt *ep;
    as_write_C_type(as, info, DevMarkInfo);
    if(info->priv) {
	ep = *(NextGSExt **)info->priv;
	as_write_C_type(as, &ep, NextGSExt *);
	if ( ep )
	    as_write_C_type(as, ep, NextGSExt);
    }
}
    


/*
  Unpacking Routines
 */

static DevRun *
unpackRuns(ReadArgStream *as, int n)
{
    DevRun *first, *run;

    first = run = as_read(as, n*sizeof(DevRun));
    while(--n >= 0 ) {
	run->data = as_read(as, LongWordPad(run->datalen*sizeof(DevShort)));
	if(run->indx)
	    run->indx = as_read(as, LongWordPad(BytesForRunIndex(run)));
	run++;
    }
    return(first);
}

static DevMask *
unpackMasks(ReadArgStream *as, int n)
{
    DevMask *first, *mask;
    MaskRec *mr;
    int maskbytes;
    
    first = mask = as_read(as, n*sizeof(DevMask));
    while(--n >= 0) {
	mr = mask->mask = as_read(as, sizeof(MaskRec));
	maskbytes = MaskByteSize(mr);
	mr->data = as_read(as, maskbytes);
	mask++;
    }
    return(first);
}

static DevImage *
unpackImage(ReadArgStream *as, int n)
{
    DevTfrFcn *tfr;
    DevImageSource *is;
    DevImage *first, *image;

    first = image = as_read(as, n*sizeof(DevImage));
    while( --n >= 0) {
	image->info.mtx = as_read(as, sizeof(Mtx));
	image->info.trap = as_read(as,
					 image->info.trapcnt*sizeof(DevTrap));
	if ( image->transfer ) {
	    tfr = image->transfer = as_read(as, sizeof(DevTfrFcn));
	    tfr->white = (tfr->white) ? as_read(as, 256) : 0;
	    tfr->red = (tfr->red) ? as_read(as, 256) : 0;
	    tfr->green = (tfr->green) ? as_read(as, 256) : 0;
	    tfr->blue = (tfr->blue) ? as_read(as, 256) : 0;
	    tfr->ucr = (tfr->ucr) ? as_read(as, 256*sizeof(DevShort)) : 0;
	    tfr->bg = (tfr->bg) ? as_read(as, 256) : 0;
	}

	is = image->source = as_read(as, sizeof(DevImageSource));
	is->samples = as_read(as, 5*sizeof(unsigned char *));
	if (is->interleaved) 
	    is->samples[0] = as_read(as, LongWordPad(is->height*is->wbytes));
	else {
	    /* non-interleaved */
	    int i, nbytes = LongWordPad(is->height*is->wbytes);
	    for(i = 0; i < is->nComponents ; i++)
		is->samples[i] = as_read(as, nbytes);
	    if(image->unused)
		/* has alpha component as well */
		is->samples[i] = as_read(as, nbytes);
	}
	is->decode = (is->decode)
	    ? as_read(as, is->nComponents*sizeof(DevImSampleDecode))
	    : 0;
	image++;
    }
    return(first);
}

DevPrim *
unpackDevPrim(ReadArgStream *as, DevPrim *graphic)
{
    DevPrim *prv = 0, *first = 0;

    for(; graphic; graphic = graphic->next) {
	graphic = as_read(as, sizeof(DevPrim));
	if(prv)
	    prv->next = graphic;
	else
	    first = graphic;
	prv = graphic;
	switch(graphic->type) {
	case trapType:
	    graphic->value.trap =
		as_read(as, graphic->items*sizeof(DevTrap));
	    break;
	case runType:
	    graphic->value.run = unpackRuns(as, graphic->items);
	    break;
	case maskType:
	    graphic->value.mask = unpackMasks(as, graphic->items);
	    break;
	case imageType:
	    graphic->value.image = unpackImage(as, graphic->items);
	    break;
	case noneType:
	    break;
	}
    }
    return(first);
}
	

DevMarkInfo *
unpackDevMarkInfo(ReadArgStream *as)
{
    NextGSExt **epp;
    DevMarkInfo *info;
    info = as_read(as, sizeof(DevMarkInfo));
    if(info->priv) {
	info->priv = (DevPrivate *) epp =as_read(as,sizeof(NextGSExt *));
	if(*epp) {
	    *epp = as_read(as, sizeof(NextGSExt));
	 }
    }
    return(info);
}

			       
