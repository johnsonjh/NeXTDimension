
#import <mach.h>
#import <sys/message.h>

/* The fifo buffer in an argstream is a series of msg_type_long_t, data
   pairs.  These pairs alternate between inline out out-of-band char buffers.
   As writes to the stream occur, they are either copied into the current
   inline buffer, or, if they are large, an out-of-band entry is created for
   the bytes */

#define AS_BUFFERSIZE (MSG_SIZE_MAX - sizeof(msg_header_t) - sizeof(msg_type_long_t))
#define AS_INLINE_THRESHOLD (AS_BUFFERSIZE/2)
typedef struct _argstream_msg {
    msg_header_t h;
    /* fifo buffer of msg_header_long_t's and inline or out-of-line data */
    msg_type_long_t t;
    char buffer[AS_BUFFERSIZE];
} ArgStreamMsg;

typedef struct write_arg_stream {
    char *p;			/* position on current page */
    unsigned int n;		/* total bytes left on current page */
    msg_type_long_t *ct;	/* pointer to current inline msg_type */
    ArgStreamMsg msg;				/* current msg */
    void (*msg_send)(ArgStreamMsg *msg);	/* how to send the message */
    int base_id;		/* base msg's id, all continuations will
				   have base_msg_id + 1000 */
} WriteArgStream;


/* fast inline as_write for writing structures, assumes that
   sizeof(type) is always less than the inline threshold!
*/

#define as_write_C_type(as,ptype,type) if(as->n >= sizeof(type)) {    \
				        as->n -= sizeof(type);        \
				        *((type *)as->p) = *(ptype);  \
					as->p += sizeof(type);	      \
				    } else                            \
				        as_write(as, ptype, sizeof(type))

/* WriteArgStream function prototypes */

WriteArgStream *as_newWrite(msg_header_t *m, void (*msg_snd)());

/* creates new writeable argument stream.  The passed in msg_header_t
   is used as a template and copied into all msg buffers that this
   stream will send.  In it the caller should put the port and msg_id that
   are desired. To distinguish messages which are continuations of
   an argstream, 1000 is added to the msg id.  In typical situations, this
   will give robust error handling because if an error occurs during the
   parsing of an arg stream, subsequent messages on that stream can
   easily be skipped.
   */
   
void as_write(WriteArgStream *as, void *p, int n);

/* write data to an argument stream.  The data is not (necessarily) copied */

void as_flushmsg(WriteArgStream *as);

/* flush any pending message to for this arg stream. */

void as_destroyWrite(WriteArgStream *as);

/* destroys the argument stream */

/* --------------------ReadArgStreams--------------------*/

#define AS_NUMMSGSTART 5
typedef struct read_arg_stream {
    int n;		/* number of bytes left in current inline msg */
    char *p;		/* pointer to inline data */
    msg_type_long_t *ct;/* pointer to current data packet */
    ArgStreamMsg **msg;	/* array of incoming msgs */
    int nmsgs;		/* number of msgs left in array */
    int maxmsgs;	/* current capacity of msg array */
    int inLine;
    char *pend;
    void (*msg_receive)(ArgStreamMsg *msg);
} ReadArgStream;
    
    
/* ReadArgStream function prototypes */

ReadArgStream *as_newRead(ArgStreamMsg *m, void (*msg_rcv)());

/* setup an argument stream for reading.  The passed in message is used
   as the first message in the stream.  The msg_rcv function is called
   to recieve new messages */

void *as_read(ReadArgStream *as, int n);

/* `read' data from an argument stream.  A pointer to the data is passed back,
   it is not copied.
 */

void as_destroyRead(ReadArgStream *as);


/* destroys an argument stream, includes all messages (except the first
   one) and any out-of-band data associated with the messages (including
   the first one).
 */



