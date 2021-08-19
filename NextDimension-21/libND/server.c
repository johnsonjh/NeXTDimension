#include "ND/NDlib.h"
#include "ND/ND_conio.h"
#include <mach_init.h>
#include <sys/message.h>
#include <sys/msg_type.h>
#include <servers/netname.h>
#include <sys/mig_errors.h>
#include <libc.h>
#include <math.h>

/* Minimal MiG compatible reply message */
typedef struct {
	msg_header_t Head;
	msg_type_t RetCodeType;
	kern_return_t RetCode;
} Reply;

/*
 * ND_Server:
 *
 *	Process an inbound message, and possibly generate a reply message.
 *	All we really do is sniff the msg_id field for our console hacks.
 *	If the message isn't a console related item, we forward the message
 *	to the kernel server, _NDKern_server().
 */
 boolean_t
ND_Server( msg_header_t *InHeadP, msg_header_t *OutHeadP )
{
	static port_t ND_port = PORT_NULL;
	ND_conio_t *InP = (ND_conio_t *) InHeadP;
	register Reply *OutP = (Reply *) OutHeadP;
	ND_var_p handle = (ND_var_p) 0;
	int i;
	kern_return_t r;
	char buff[ND_CON_BUFSIZE];
	
	if ( ND_port == PORT_NULL )
	{
	    r = netname_look_up(name_server_port, "", "NextDimension", &ND_port);
	    if ( r != KERN_SUCCESS)
	    {
		mach_error( "netname_lookup", r );
		return FALSE;
	    }
	}

	/* Decide what to do with this message. */
	switch( InP->Head.msg_id )
	{
	    case ND_CONIO_OUT:
		fputs( InP->Data, stdout );
		break;
		
	    case ND_CONIO_IN_STRING:
		fflush( stdout );
		fgets( buff,ND_CON_BUFSIZE-1,stdin );
		r = ND_ConsoleInput(InP->Head.msg_remote_port, InP->Slot,
					buff, strlen(buff)+1 );
		if ( r != KERN_SUCCESS )
			mach_error( "ND_ConsoleInput", r );
		break;
    
	    case ND_CONIO_IN_CHAR:
		buff[0] = getchar();
		buff[1] = 0;
		r = ND_ConsoleInput(InP->Head.msg_remote_port, InP->Slot, buff, 1);
		if(r != KERN_SUCCESS)
			mach_error( "ND_ConsoleInput", r );
		break;
		
	    case ND_CONIO_EXIT:
		printf("NextDimension board (Slot %d) has halted.\n",InP->Slot);
		return FALSE;
		break;
		
	    case ND_CONIO_MESSAGE_AVAIL:
		return TRUE;		/* We got one! */
    
	    default:
		return( _NDKern_server( InHeadP, OutHeadP ) );
	}
	OutP->RetCode = MIG_NO_REPLY;
	return TRUE;
}

