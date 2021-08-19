/*****************************************************************************
	
    nd.h
    Header file for NextDimension Device Driver

    CONFIDENTIAL
    Copyright 1990 NeXT Computer, Inc. as an unpublished work.
    All Rights Reserved.

    Created 27Feb89 Ted Cohn

    Modifications:
 
******************************************************************************/

#define ND_ROMID	1024	/* Unique ROM id for product */
#define ND_WIDTH	1120	/* Pixels wide (visible pixels) */
#define ND_HEIGHT	832	/* Pixels high (visible scanlines)*/
#define ND_ROWBYTES	4608	/* Row bytes to start of next line */

/* Private state information cached in the NXSDevice priv field. */

typedef struct _ND_PrivInfo_ {
    vm_address_t fb_addr;	/* Virtual addr of frame buffer base */
    vm_size_t	 fb_size;	/* Size of mapped frame buffer in bytes */
    port_t	 server_port;	/* Port to server code. */
    port_t	 kernel_port;	/* Port to i860 server code. */
    port_t	 reply_port;	/* Port i860 replies to us on. */
} ND_PrivInfo;
