#include "ND/NDlib.h"
#include "ND/ND_conio.h"
#include <sys/message.h>
#include <sys/msg_type.h>
#include <sys/mig_errors.h>

/* Minimal MiG compatible reply message */
typedef struct {
	msg_header_t Head;
	msg_type_t RetCodeType;
	kern_return_t RetCode;
} Reply;

/*
 * _NDKern_server:
 *	Default kernel server stub, used when the user doesn't provide one.
 *	Just consume the message and don't bother with anything else.
 */
 boolean_t
 _NDKern_server( msg_header_t *InHeadP, msg_header_t *OutHeadP )
{
	register Reply *OutP = (Reply *) OutHeadP;

	OutP->RetCode = MIG_NO_REPLY;
	return TRUE;
}
