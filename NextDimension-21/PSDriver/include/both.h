/*****************************************************************************
	
    both.h
    Header file shared between NextDimension client and server

    CONFIDENTIAL
    Copyright 1990 NeXT Computer, Inc. as an unpublished work.
    All Rights Reserved.

    20Aug90 Ted Cohn

    Modifications:
 
******************************************************************************/

#define PSMAKEPUBLICID	100
#define PSMARKID	101

/* Stucture used for forming request message for makePublic method */
typedef struct {
    msg_header_t h;
    msg_type_t ints;
    int rbm;
    int hintDepth;
    Bounds hintBounds; 
} MakePublicReq;

/* Stucture used for forming reply message from makePublic method */
typedef struct {
    msg_header_t h;
    msg_type_t ints;
    int type;
    int rowBytes;
    Bounds bounds;
    msg_type_long_t oobBits;
    char *bits;
    msg_type_long_t oobAbits;
    char *abits;
} MakePublicReply;

/* Structure to wad up top level Mark arguments */
typedef struct {
    int bm_addr;		/* addr of local bitmap */
    MarkRec mr;			/* Mark args */
    Bounds markBds;
    Bounds bpBds;
} MarkRequest;

/* helpers for packing/unpacking mark arguments (in markpack.c) */
#import "argstream.h"
void packDevPrim(WriteArgStream *as, DevPrim *graphic);
void packDevMarkInfo(WriteArgStream *as, DevMarkInfo *info);
DevPrim *unpackDevPrim(ReadArgStream *as, DevPrim *graphic);
DevMarkInfo *unpackDevMarkInfo(ReadArgStream *as);
