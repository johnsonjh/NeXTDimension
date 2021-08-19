/*****************************************************************************

    argstream.c
    Process argument streams for sending over mach messages

    CONFIDENTIAL
    Copyright 1990 NeXT Computer, Inc. as an unpublished work.
    All Rights Reserved.

    25Aug90 Peter Graffagnino

    Modified:

******************************************************************************/

#include <stdlib.h>
#include <string.h>
#include "argstream.h"

static const msg_type_long_t CharsTypeTemplate = {
    { 0, 0, 0, 1, 1, 0}, MSG_TYPE_CHAR, 8, 0
    };

typedef struct _oobDataTemplate {
    msg_type_long_t mt;
    char *data;
    msg_type_long_t it;
} oobDataTemplate;

/* this template is written on */
static oobDataTemplate OOCharsDataTemplate = {
    {{ 0, 0, 0, 0, 1, 0}, MSG_TYPE_CHAR, 8, 0}, 0,
    {{ 0, 0, 0, 1, 1, 0}, MSG_TYPE_CHAR, 8, 0}
};

#ifdef DEBUG
void
as_dumpMessage(ArgStreamMsg *m)
{
    int data_skip_size;
    int i = 0;
    msg_type_long_t *ct = &m->t;
    char *msgend = (char *)m + m->h.msg_size;

    printf("ArgSreamMsgDump: (m = %X)\n", m);
    printf("Header:\n");
    printf("        msg_simple: (%s)\n",m->h.msg_simple ? "true" : "false");
    printf("            msg_id: (%D)\n",m->h.msg_id);
    printf("          msg_size: (%D)\n",m->h.msg_size);
    printf("\n");

    while((char *)ct < msgend) {
	printf("Buffer %D\n",++i);
	printf("           inline: (%s)\n",
	       ct->msg_type_header.msg_type_inline ? "true": "false");
	printf("           number: (%D)\n", ct->msg_type_long_number);
	if(!ct->msg_type_header.msg_type_inline) {
	    data_skip_size = sizeof(char *);
	    printf("           oobptr: (%X)\n", *(void **)(ct + 1));
	} else {
	    data_skip_size = (ct->msg_type_long_number + 3) &~3;
	}
	/* advance to the next msg_type_long_t */
	ct = (msg_type_long_t *) ((char *)(ct+1) + data_skip_size);
	printf("\n");
    }

}
    
void
as_dumpRead(ReadArgStream *as)
{
    int i,k;
    
    printf("ReadArgStream dump (as = %X)\n", as);
    printf("            n: (%D)\n", as->n);
    printf("            p: (%X)\n", as->p);
    printf("           ct: (%X)\n", as->ct);
    printf("        nmsgs: (%D)\n", k = as->nmsgs);
    printf("      maxmsgs: (%D)\n", as->maxmsgs);
    printf("       inLine: (%s)\n", as->inLine ? "true" : " false");
    printf("         pend: (%X)\n", as->pend);
    printf("  msg_receive: (%X)\n", as->msg_receive);
    for(i = 0; i < k; i++)
	as_dumpMessage(as->msg[i]);
}
    
#endif

static void
as_error(char *s)
{
    os_raise(0,s);
}

static void
as_init(WriteArgStream *as)
{
    /* reset the message and buffer pointers */
    as->p = as->msg.buffer;
    as->n = AS_BUFFERSIZE;
    as->ct = &as->msg.t;
    as->msg.t = CharsTypeTemplate;
    as->msg.h.msg_simple = 1;	/* starts out 1 */
}

    

WriteArgStream *
as_newWrite(msg_header_t *msg_header, void (*msg_send)())
{
    WriteArgStream *as = malloc(sizeof(WriteArgStream));

    if(!as) as_error("malloc failed");
    /* copy in msg header */
    as->msg.h = *msg_header;
    as->base_id = msg_header->msg_id;
    as->msg_send = msg_send;
    as_init(as);
    return(as);
}
    

						    
void
as_flushmsg(WriteArgStream *as)
{
    /* make sure there is actually data in the buffer */
    if(as->p == as->msg.buffer)
	return;
    /* if there is any data in the last inline packet, store its size
       in its msg_type_long_t.  Otherwise subtract off the last
       msg_type_long_t.
       */
    if(as->p == (char *) (as->ct + 1))
	/* only tweak p (we wont need n anymore) */
	as->p -= sizeof(msg_type_long_t);
    else
	as->ct->msg_type_long_number = as->p - (char *)(as->ct + 1);
    /* finally fill out the total size for the entire msg */
    as->msg.h.msg_size = as->p - (char *)&as->msg;

    /* send the message */
    (as->msg_send)(&as->msg);
    as_init(as);
    /* all subsequent messages will have 1000 added to their id's */
    as->msg.h.msg_id = as->base_id + 1000;
}

    

static inline void
as_align(WriteArgStream *as)
{
    /* assume that this adjustment will not require more memory, i.e.
       fifo buffer is always a multiple of 4 bytes */
    if((int)as->p & 3) {
	int n = 4 - ((int)as->p & 3);
	as->p += n;
	as->n -= n;
    }
}

#define as_put_C_type(as, ptype, type) *((type *)as->p) = *(ptype), \
					    as->p += sizeof(type),  \
						as->n -= sizeof(type)

void
as_write(WriteArgStream *as, void *p, int n)
{
    if(n <= AS_INLINE_THRESHOLD) {
	/* data will go inline, update current msg_type_long_t and write the
	   data */
	if(as->n < n) 
	    as_flushmsg(as);
	as->n -= n;
	bcopy((char *)p, as->p, n);
	as->p += n;
    } else {
	/* data will be passed out-of-line. if there's not at least enough
	   room for a OOCharsDataTemplate, go ahead and flush the stream */
	
	if(as->n < sizeof(OOCharsDataTemplate))
	    as_flushmsg(as);
	   
	/* data will be passed out-of-line.  If the current inline packet has
	   no data in it, rewind and write over it, otherwise align the stream
	   for a new msg_type_long_t */
	if(as->p == (char *)(as->ct + 1)) {
	    /* we need to ensure that the out-of-line type_t and data are
	       written with one atomic write to the stream (so that they are
	       not split over msgs.) To do this we rewind the stream. */
	    as->p -= sizeof(msg_type_long_t);
	    as->n += sizeof(msg_type_long_t);
	} else {
	    /* otherwise, we need to fill in the size of the inline packet
	       and advance the stream to the nearest long-word boundary */
	    as->ct->msg_type_long_number = as->p - (char *)(as->ct + 1);
	    as_align(as);
	}
	/* create new msg_type_long_t for the out-of-band data */
	OOCharsDataTemplate.mt.msg_type_long_number = n;
	if((int)p & 3) {
	    /* oh no, unaligned data, create some new memory and
	       bcopy it (sigh). (All buffers must arrive long word
	       aligned)
	       */
	    vm_offset_t vptr;
	    if(vm_allocate(task_self(), &vptr,(vm_size_t) n, 1))
		as_error("vm_allocate failed\n");
	    bcopy(p,(void*)vptr,n);
	    OOCharsDataTemplate.data = (void*)vptr;
	    OOCharsDataTemplate.mt.msg_type_header.msg_type_deallocate = 1;
	} else {
	    OOCharsDataTemplate.data = p;
	    OOCharsDataTemplate.mt.msg_type_header.msg_type_deallocate = 0;
	}
	as_put_C_type(as, &OOCharsDataTemplate, oobDataTemplate);
	as->msg.h.msg_simple = 0;
	/* finally, start off a new inline template */
	as->ct = (msg_type_long_t *)(as->p - sizeof(msg_type_long_t));
    }
}

void 
as_destroyWrite(WriteArgStream *as)
{
    free(as);
}

	

ReadArgStream *
as_newRead(ArgStreamMsg *m, void (*msg_receive)())
{
    ReadArgStream *as = calloc(1,sizeof(ReadArgStream));

    if(!as) as_error("as_newRead: no memory");

    as->maxmsgs = AS_NUMMSGSTART;
    as->msg = calloc(AS_NUMMSGSTART, sizeof(ArgStreamMsg *));
    if(!as->msg) as_error("as_newRead: no memory");
    as->msg[0] = m;
    as->nmsgs = 1;
    as->p = m->buffer;
    as->n = m->t.msg_type_long_number;
    as->ct = &m->t;
    as->inLine = as->ct->msg_type_header.msg_type_inline;
    as->pend = (char *)m + m->h.msg_size;
    as->msg_receive = msg_receive;
#ifdef DEBUG
    printf("as_newRead, dumping argstream\n");
    as_dumpRead(as);
#endif
    return(as);
}


static void
as_new_message(ReadArgStream *as)
{
    ArgStreamMsg *newmsg;
    if(++as->nmsgs == as->maxmsgs) {
	/* grow the message array */
	as->maxmsgs += as->maxmsgs;
	as->msg = realloc(as->msg, as->maxmsgs*sizeof(ArgStreamMsg *));
    }
    /* allocate a new msg */
    newmsg = malloc(sizeof(ArgStreamMsg));
    /* copy in the msg_header_t */
    newmsg->h = as->msg[0]->h;
   
    (*as->msg_receive)(newmsg);
    as->msg[as->nmsgs-1] = newmsg;
    /* verify that it is a continuation of the first message */
    if(newmsg->h.msg_id != as->msg[0]->h.msg_id + 1000)
	as_error("as_new_message: incorrect id for continuation message");
    as->p = newmsg->buffer;
    as->n = newmsg->t.msg_type_long_number;
    as->ct = &newmsg->t;
    as->inLine = as->ct->msg_type_header.msg_type_inline;
    as->pend = (char *)newmsg + newmsg->h.msg_size;
#ifdef DEBUG
    printf("as_new_message, dumping argstream\n");
    as_dumpRead(as);
#endif
}


void *
as_read(ReadArgStream *as, int n)
{
    void *p;
    if(n > as->n) {
	/* every read should align exactly */
	if(as->n != 0) {
#ifdef DEBUG
	    printf("size mismatch: in-stream %D, requested %D\n", as->n, n);
	    as_dumpRead(as);
#endif
	    as_error("as_read: size mismatch on read");
	}
	/* don't have enough data in current buffer, first check if
	   we need to go off and read another message */
	if(as->p == as->pend) 
	    /* end of message, read in another one */
	    as_new_message(as);
	else {
	    /* go get the next msg_type_long_t */
	    as->ct = (msg_type_long_t *) ((!as->inLine) ?
		((char *)(as->ct+1) + sizeof(char *)) :
		((char *)(as->ct+1) + ((as->ct->msg_type_long_number+3)&~3)));
	    as->p = (char *)as->ct + sizeof(msg_type_long_t);
	    as->n = as->ct->msg_type_long_number;
	    as->inLine = as->ct->msg_type_header.msg_type_inline;
	}
    }
    if(as->inLine) {
	p = as->p;
	as->p += n, as->n -= n;
    } else {
	if(n != as->n)	{
#ifdef DEBUG
	    printf("oob size mismatch: in-stream %D, requested %D\n");
	    as_dumpRead(as);
#endif
	    as_error("as_read: size mismatch on out-of-band read");
	}
	p = *((void **)as->p);
	as->n = 0;
    }
    return(p);
}


/* destroys any out-of-band data associated with the message */

static inline void as_destroyMsg(ArgStreamMsg *m)
{
    int data_skip_size;
    if(!m->h.msg_simple) {
	/* need to walk the message looking for out-of-band data */
	msg_type_long_t *ct = &m->t;
	char *msgend = (char *)m + m->h.msg_size;
	while((char *)ct < msgend) {
	    if(!ct->msg_type_header.msg_type_inline) {
		/* do the vm_deallocate */
		vm_address_t a = *((vm_address_t *)(ct + 1));
		vm_deallocate(task_self_, a,
			      (vm_size_t) ct->msg_type_long_number);
		data_skip_size = sizeof(char *);
	    } else {
		data_skip_size = (ct->msg_type_long_number + 3) &~3;
	    }
	    /* advance to the next msg_type_long_t */
	    ct = (msg_type_long_t *) ((char *)(ct+1) + data_skip_size);
	}
    }
}

/* destroy does *not* free the first message i.e. the one passed to
   as_newRead, but *does* free its inline data  */

void
as_destroyRead(ReadArgStream *as)
{
    int i;
    ArgStreamMsg **mp;
    
    for(i = 0, mp = as->msg; i < as->nmsgs; i++, mp++) {
	as_destroyMsg(*mp);
	if(i) free(*mp); /* don't free the first one! */
    }
    
    free(as->msg);
    free(as);
}

