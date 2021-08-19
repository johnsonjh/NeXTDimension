/*****************************************************************************

    server.c
    Main server loop receives and distributes messages
    
    CONFIDENTIAL
    Copyright 1990 NeXT Computer, Inc. as an unpublished work.
    All Rights Reserved.

    01Aug90 Ted Cohn
    
    Modifications:

******************************************************************************/

#import <mach.h>
#import <sys/message.h>
#import <mig_errors.h>
#import <ND/NDlib.h>

#include "vm/vm_param.h"
#include "vm/vm_prot.h"

/*
#include "i860/vm_types.h"
#import "ND/NDmsg.h"
#import "ND/ND_types.h"
#import "i860/proc.h"
*/

#import "package_specs.h"
#import "bitmap.h"
#import "except.h"
#import "both.h"
#import "server.h"


void sendreply(msg_header_t *out_msg)
{
    kern_return_t r;
    r = msg_send( out_msg, MSG_OPTION_NONE, 0 );
    if (r != KERN_SUCCESS) {
	printf( "msg_send returns %D\n", r );
    }
}

main(int argc, char **argv, char **envp)
{
    typedef struct {
	msg_header_t Head;
	msg_type_t RetCodeType;
	kern_return_t RetCode;
    } Reply;
    Reply *out_msg;
    msg_header_t *msg;
    kern_return_t r;
    extern void * malloc();
    
    SupportInit();
    NDDeviceInit();
    video_init();
    msg = (msg_header_t *) malloc( MSG_SIZE_MAX );
    out_msg = (Reply *) malloc( MSG_SIZE_MAX );
    
    if ( msg == (msg_header_t *)0 || out_msg == (Reply *)0 ) {
    	printf( "malloc fails\n");
	exit(1);
    }
    r = port_allocate( task_self(), &msg->msg_local_port );
    if ( r != KERN_SUCCESS ) {
	printf( "port_allocate returns %D\n", r );
	exit( 0 );
    }
#if !defined(i860)
    r = netname_check_in( name_server_port, "ps_server", (port_t) 0,
	msg->msg_local_port );
    if ( r != KERN_SUCCESS ) {
	printf( "netname_check_in returns %D\n", r );
	exit( 0 );
    }
#else
    r = ND_Port_check_in( task_self(), "ps_server", msg->msg_local_port );
    if ( r != KERN_SUCCESS )
    {
	printf( "i860 ND_Port_check_in returns %D\n", r );
	exit( 0 );
    }
#endif
    printf( "Registered port %D\n", msg->msg_local_port );
    while(1) {
	DURING
	while (1) {
	    msg->msg_size = MSG_SIZE_MAX;
	    r = msg_receive( msg, MSG_OPTION_NONE, 0 );
	    if (r != KERN_SUCCESS) {
		printf( "msg_receive returns %D\n", r );
		continue;
	    }
	    /* in mig-generated psServer.c */
	    switch(msg->msg_id) {
	    case PSMAKEPUBLICID:
	        ps_rbm_makePublic(msg);
	        break;
	    case PSMARKID:
		ps_rbm_mark(msg);
		break;
	    default:
		ps_server(msg, (msg_header_t*)out_msg);
		if (out_msg->RetCode != MIG_NO_REPLY)
		    sendreply((msg_header_t *) out_msg);
	    }
	}
	HANDLER {
	    printf("Uncaught exception: %d  %s\n", Exception.Code,
		   (Exception.Message == NIL) ? "" : Exception.Message);
	    /* cleanup mark argstream if necessary */
	    MarkExceptionCleanup();
	} END_HANDLER
    }
}

