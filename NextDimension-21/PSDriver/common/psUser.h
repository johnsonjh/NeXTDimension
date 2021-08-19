
/*****************************************************************************

    psUser.h
    definition to be included in psUser.c

    CONFIDENTIAL
    Copyright 1990 NeXT Computer, Inc. as an unpublished work.
    All Rights Reserved.

    03Sep90 Peter Graffagnino
        
    Modifications:

******************************************************************************/

/* Hack in our own msg send routine */
#define msg_send ps_msg_send
