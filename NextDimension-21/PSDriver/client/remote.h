/*****************************************************************************
	
    remote.h
    Header file for remote bitmaps

    CONFIDENTIAL
    Copyright 1990 NeXT Computer, Inc. as an unpublished work.
    All Rights Reserved.

    01Aug90 Ted Cohn

    Modifications:
 
******************************************************************************/

#import <mach.h>
#import "package_specs.h"
#import "bitmap.h"
#import "mp12.h"

typedef struct _RemoteBitmap {
    Bitmap base;	/* super class */
    port_t port;	/* server port of device that owns this bitmap */
    int addr;		/* address of remote bitmap */
} RemoteBitmap;

typedef struct _RemoteBMClass {
    BMClass base;
    Bitmap *(*newRemote)(int type, int special, port_t port, Bounds *b);
    Bitmap *(*remoteFromData)(int type, port_t port, Bounds *b,
	unsigned int *bits, unsigned int *abits, int byteSize, int rowBytes);
} RemoteBMClass;

#define bm_newRemote(type,special,port,bounds)\
	(*_remoteBM.newRemote)(type,special,port,bounds)

#define bm_remoteFromData(type,port,bounds,bits,abits,size,rowBytes)\
	(*_remoteBM.newRemote)(type,port,bounds,bits,abits,size,rowBytes)

extern RemoteBMClass _remoteBM;

#define remoteBM ((BMClass *)&_remoteBM)

extern int nd_synchronous;
