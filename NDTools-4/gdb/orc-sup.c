#ifdef ORC			/* ORC-only file */

/* some support routines for gdb that aren't in libc.a yet! */
/* CB, ORC, Jan 89 */

typedef struct qel {
  struct qel *forward, *backward;
  char *data;
} qelem;

#define NULL (qelem *) 0

/* queues are initialized by allocating one struct of type qelem */
/* and having forward == backward == qelem */

insque( elem, pred)
register qelem *elem, *pred;
/* insert elem after pred */
{
  if ( elem == NULL || pred == NULL) return;
  elem->backward = pred; elem->forward = pred->forward;
  pred->forward->backward = elem; pred->forward = elem;
}

remque( elem)
register qelem *elem;
{
  if ( elem->forward == elem->backward) return; /* queue empty */
  elem->forward->backward = elem->backward;
  elem->backward->forward = elem->forward;
  elem->forward = elem->backward = NULL;
}


#include <stdio.h>
#include <varargs.h>

extern int _doprnt();

/*VARARGS1*/
int
vprintf(format, ap)
char *format;
va_list ap;
{
	register int count;

	if (!(stdout->_flag & _IOWRT)) {
		/* if no write flag */
		if (stdout->_flag & _IORW) {
			/* if ok, cause read-write */
			stdout->_flag |= _IOWRT;
		} else {
			/* else error */
			return EOF;
		}
	}
	count = _doprnt(format, ap, stdout);
	return(ferror(stdout)? EOF: count);
}

#include <sys/param.h>
#include <sys/dir.h>
#include <signal.h>
#include <sys/user.h>
#include <sys/ioctl.h>
#include <fcntl.h>

#include <a.out.h>
#include <sys/file.h>
#include <sys/stat.h>

#include <machine/reg.h>

DumpUser( u)
register struct user *u;
{ int fd;

  fd = open( "user_struct", O_RDWR|O_CREAT, 0666);
  write( fd, u, sizeof( struct user));
  close( fd);
}

#endif ORC
