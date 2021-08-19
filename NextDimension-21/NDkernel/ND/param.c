#include <sys/map.h>
#include <i860/param.h>
#include "i860/proc.h"

struct map *coremap;

struct proc proc[NPROC];
struct proc *P;		/* The current process */

/* Pager control variables */
unsigned long FreePageCnt = 0;
unsigned long MinFreePages = 64;

/* Assorted globals */
int	runrun;		/* The rescheduling flag. True if a higher priority */
			/* process is waiting. */
int	runin;		/* True if scheduler is waiting to run. */
int	curpri;		/* Priority of currently executing process */
unsigned long           _slot_id_;  /* (NextBus slot << 28), used in macros */
