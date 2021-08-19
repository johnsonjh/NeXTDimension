#include <mach.h>
#include <servers/netname.h>
#include <stdio.h>
#include "ND/NDlib.h"

main( argc, argv )
    int argc;
    char **argv;
{
    extern struct mach_header _mh_execute_header;
    port_t ND_port;
    port_t debug_port = PORT_NULL;
    char * program = argv[0];
    char *prog860 = "a.out";
    int slot = -1;
    int slotbits, i;
    kern_return_t r;
    
    while ( --argc )
    {
    	if ( **(++argv) == '-' )
	{
		if ( strcmp( *argv, "-s" ) == 0 && argc > 1)
		{
		    --argc;
		    slot = atoi( *(++argv) );
		}
		else
		    usage( program );
	}
	else
	    prog860 = *argv;
    }
    r = netname_look_up(name_server_port, "", "NextDimension", &ND_port);
    if (r != KERN_SUCCESS)
    {
    	mach_error( "netname_lookup", r );
	exit( 1 );
    }

    r = ND_GetBoardList( ND_port, &slotbits );
    if (r != KERN_SUCCESS)
    {
    	mach_error( "ND_GetBoardList", r );
	exit( 1 );
    }
    if ( slot == -1 )
    {
    	for ( i = 0; i < SLOTCOUNT*2; i += 2 )
	{
	    if ( ((1 << i) & slotbits) != 0 )
	    {
	    	slot = i;
		break;
	    }
	}
	if ( slot == -1 )
	{
	    printf( "No NextDimension board found.\n" );
	    exit( 1 );
	}
    }
    else
    {
    	if ( ((1 << slot) & slotbits) == 0 )
	{
	    printf( "No NextDimension board in Slot %d.\n", slot );
	    exit( 1 );
	}
    }
    /*
     * Run860 supports debugging, bind860 doesn't...
     * This keeps demo hackers happy.
     */
#if defined(DEBUG860) || defined(RUN860)
    /* Allocate a debug port to handle console I/O for the ND board. */
    if ( (r = port_allocate( task_self(), &debug_port)) != KERN_SUCCESS)
    {
    	mach_error( "port_allocate", r );
	exit( 1 );
    }
    /* Conect the debug port to the ND device driver. */
    if ( (r = ND_SetDebug( ND_port, slot, debug_port)) != KERN_SUCCESS)
    {
    	mach_error( "ND_SetDebug", r );
	exit( 1 );
    }
#endif
#if defined(BIND860)
    r = ND_BootKernelFromSect(ND_port, slot, &_mh_execute_header, "__I860", "__i860");
    if ( r != KERN_SUCCESS )
    {
    	mach_error( "ND_BootKernelFromSect", r );
	exit( 1 );
    }
#endif
#if defined(RUN860)
    r = ND_BootKernel( ND_port, slot, prog860 );
    if ( r != KERN_SUCCESS )
    {
    	mach_error( "ND_BootKernel", r );
	exit( 1 );
    }
    fprintf( stderr, "Launching %s (slot %d)\n", prog860, slot );
#endif
#if defined(DEBUG860)
    printf( "Monitoring NextDimension board in Slot %d.\n", slot );
#endif
#if defined(DEBUG860) || defined(RUN860)
    ND_DefaultServer( ND_port, debug_port );
#endif
#if defined(BIND860)
    WaitForParent();
#endif
    ND_Close( ND_port, slot );
    exit( 0 );
}

#if defined(BIND860)
WaitForParent()
{
	int ppid;
	
	ppid = getppid();
	/* 
	 * Sleep, checking at intervals to see if the parent has gone
	 * away.  This is a crock.
	 */
	do
	{
		sleep( 5 );
	}
	while ( kill( ppid, 0 ) == 0 );
}
#endif


usage( char * program )
{
#if defined(RUN860)
	printf( "Usage: %s [-s Slot] [i860 program]\n", program );
#else
	printf( "Usage: %s [-s Slot]\n", program );
#endif
	exit( 1 );
}
