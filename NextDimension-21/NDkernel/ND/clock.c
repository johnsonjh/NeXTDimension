#include "i860/trap.h"
#include "i860/proc.h"
#include <sys/map.h>
#include "ND/NDmsg.h"
#include "ND/NDreg.h"

int ticks = 0;
int lbolt = 0;		/* The once a second wakeup pinger */

extern unsigned long FreePageCnt;
extern unsigned long MinFreePages;
extern struct map *coremap;

int DebugInitialized = 0;
int		_DebugEnabled_ = 1;

 void
EnableDebugging()
{
	_DebugEnabled_ = 1;
}

 void
DisableDebugging()
{
	_DebugEnabled_ = 0;
}


/*
 * Dispatch code for periodic events, timing, and whatnot.
 */
hardclock( locr0 )
    int *locr0;
{
	static int next_lbolt = 60;
	
	++ticks;

	CursorTask();	/* Run cursor task every retrace interval */

	/*
	 * About 4 times a second, check for exhaustion of the free page
	 * list, and wake the coremap page out thread if needed.
	 */
	if ( (ticks & 0xF) == 0 && FreePageCnt < MinFreePages )
		Wakeup( &coremap->m_name );

	if ( ticks >= next_lbolt )	/* Run once a second code */
	{
		/* Post the once a second wakeup(). */
		++lbolt;
		next_lbolt += 60;
		Wakeup( &lbolt );	/* Anyone sleeping on lbolt? */
		
		/*
		 * Mach has been known to drop an occasional interrupt.
		 * Make sure that the host doesn't get stuck by pinging it
		 * once a second if there is data in the FromND queue, and no pending
		 * interrupt on the host side.
		 */
		if ( MSG_QUEUES->FromND.Head != MSG_QUEUES->FromND.Tail
		    && ! ND_IS_INTCPU )
		    ping_driver();		/* There is work to be done. */
		
		/*
		 * If the debugger has just been connected, drop into the kdb monitor
		 * to service any initial requests.  The monitor sets DebugInitialized.
		 *
		 * If the debugger is no longer connected, clear the flag so we can
		 * cleanly reconnect the next time around.
		 */
		if ( _DebugEnabled_ )	/* Permit app to disable debug connections */
		{
		    if ( ! DebugInitialized )
		    {
			if ( KernelDebugEnabled() )
			    kdb( 16, locr0, 0 ); /* 16 is the 'enter KDB' trap */
		    }
		    else
		    {
			if ( ! KernelDebugEnabled() )
			    DebugInitialized = 0;
		    }
		}	
	}
}

