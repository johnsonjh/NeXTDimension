#include "ND/NDlib.h"
#include "ND/ND_conio.h"
#include <stdio.h>
#include <mach.h>
#include <mach_error.h>
#include <mig_errors.h>
#include <sys/message.h>
#include <sys/notify.h>
#include <libc.h>
#include <math.h>

#define ND_MSG_SPACE \
    (MSG_SIZE_MAX - (sizeof(msg_header_t)+sizeof(msg_type_t)+sizeof(kern_return_t)))

/*
 * ND_DefaultServer:
 *
 *	Accept messages from a remote source and dispatch them through a MiG server
 *	interface.
 */
 void
ND_DefaultServer( port_t ND_port, port_t localport )
{ 
	typedef struct DumMsg 
	{ 
		msg_header_t	head; 
		msg_type_t	retcodetype; 
		kern_return_t	return_code; 
		char 		space[ND_MSG_SPACE]; 
	} DumMsg;

	kern_return_t	retcode; 
	msg_return_t	msgcode; 
	boolean_t	ok; 
	DumMsg		*pInMsg, *pRepMsg;
	int i;
	
	pInMsg = (DumMsg *)malloc(sizeof(DumMsg)); 
	pRepMsg = (DumMsg *)malloc(sizeof(DumMsg));

	while (TRUE) 
	{ 
	    pInMsg->head.msg_size = sizeof(DumMsg);	/* bytes */ 
	    pInMsg->head.msg_local_port = localport;

	    /* wait to receive request from client */ 
	    msgcode = msg_receive(&pInMsg->head,MSG_OPTION_NONE,0);
	    
	    if (msgcode != RCV_SUCCESS) 
		printf("error %s in Receive, message will be ignored.\n", 
			mach_errormsg((kern_return_t)msgcode)); 
	    else 
	    {	if (pInMsg->head.msg_type == MSG_TYPE_EMERGENCY) 
		{ 
		    if (pInMsg->head.msg_id == NOTIFY_PORT_DELETED) 
		    	break;	/* All done. The client died. */ 
		    else 
			printf("Unexpected emergency message received: id is %d\n", 
				        pInMsg->head.msg_id); 
		} 
		else		/* normal message */ 
		{ 
		    /* call server interface module */ 
		    if ( pInMsg->head.msg_remote_port == PORT_NULL )
		    	pInMsg->head.msg_remote_port = ND_port;
		    ok = ND_Server((msg_header_t *)pInMsg,(msg_header_t *)pRepMsg);
		    /* The client died or we hit a fatal error state */
		    if ( ok == FALSE )
		        break;
			
		    if (pRepMsg->return_code != MIG_NO_REPLY) 
		    { 
			/* sending reply message to client */ 
			pRepMsg->head.msg_local_port = 
				pInMsg->head.msg_local_port; 
			pRepMsg->head.msg_remote_port = 
				pInMsg->head.msg_remote_port; 
			msgcode = msg_send(&pRepMsg->head,MSG_OPTION_NONE,0);
			if ((msgcode != SEND_SUCCESS) && 
				(msgcode != SEND_INVALID_PORT)) 
				/* Probably remote process death */ 
				printf("error %s at Send.\n", 
					mach_errormsg((kern_return_t)msgcode)); 
		    } 
		}	/* normal message */ 
	    }		/* of message handling */ 
	}		/* of main loop */ 
	free( (char *)pInMsg ); 
	free( (char *)pRepMsg );
}
