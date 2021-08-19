#include <stdio.h>
#include <math.h>
#include <ctype.h>
#include <sys/ioctl.h>
#include <ND/NDlib.h>
#include <mach_init.h>
#include <sys/file.h>
#include <signal.h>
#include <setjmp.h>
#include <libc.h>

static void	SetGamma( int, unsigned char * );
static void usage( char * );
static void thud( int );

jmp_buf	Whoops;

port_t ND_port;
port_t debug_port;

main( argc, argv )
    int argc;
    char **argv;
{
    int slot = -1;
    double UseGamma;
    char * program = argv[0];
    void (*old_SIGBUS)();
    void (*old_SIGSEGV)();
    unsigned char table[256];
    int slotbits;
    int i;
    kern_return_t r;
    
    UseGamma = 2.3;
    while (--argc)
    {
	if (*(*++argv) == '-')
	{
	    switch (*((*argv)+1))
	    {
		case 's':
		    if (*((*argv)+2))
			slot = atoi( (*argv)+2 );
		    else
			slot = atoi( *++argv ), --argc;
		    break;

		default:
		    fprintf( stderr, "Unknown option %s\n", *argv );
		    usage( program );
		    exit( 1 );
		    break;
	    }
	}
	else
	{
		UseGamma = atof( *argv );
	}
    }
    
    // Get the port for the NextDimension driver
    r = netname_look_up(name_server_port, "", "NextDimension", &ND_port);
    if (r != KERN_SUCCESS)
    {
    	mach_error( "netname_lookup(NextDimension)", r );
	exit( 1 );
    }

    // Find out what boards are actually present in the system
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
    /* Allocate a debug port. */
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
    
    // Build the gamma table up.
    for ( i = 0; i < (sizeof table / sizeof table[0]); ++i )
    {
    	table[i] = (unsigned int)
		floor((pow(((double)i/255.0),(1.0 / UseGamma))*255.0)+ 0.5);
    }
    
    old_SIGBUS = signal( SIGBUS, thud );
    old_SIGSEGV = signal( SIGSEGV, thud );
    
    if ( setjmp(Whoops) != 0 )
    {
	    (void) signal( SIGBUS, old_SIGBUS );
	    (void) signal( SIGSEGV, old_SIGSEGV );
	    fprintf( stderr, "%s: hardware access error.\n", program );
	    exit( 1 );
    }
    
    SetGamma( slot, table );
    UnBlank( slot );

    (void) signal( SIGBUS, old_SIGBUS );
    (void) signal( SIGSEGV, old_SIGSEGV );

    exit(0);
}

 static void
usage( char * prog )
{
	fprintf( stderr, "usage: %s ", prog );
	fprintf( stderr, "[-s slot] [gamma]\n" );
	exit( 1 );
}


 void
SetGamma(int slot, unsigned char * table )
{
	vm_offset_t	Addr;		// Address of the board as mapped
	vm_size_t	size;
	kern_return_t	r;
	volatile unsigned char *raddr;
	int index, color;
	
	r = ND_MapBT463( ND_port, debug_port, task_self(), slot, &Addr, &size );
	if ( r != KERN_SUCCESS )
	{
		mach_error( "ND_MapBT463", r );
		exit( 1 );
	}
	raddr = (volatile unsigned char *)Addr;

	// Load up command register 0, 0x201
	raddr[0] = 0x01;
	raddr[4] = 0x02;
		raddr[8] = 0x40;	// 1:1 interleave
	// Command register 1, 0x202
	raddr[0] = 0x02;
	raddr[4] = 0x02;
		raddr[8] = 0x00;	// ??
	// Command register 2, 0x203
	raddr[0] = 0x03;
	raddr[4] = 0x02;
		raddr[8] = 0x80;	// Enable sync, no pedestal
	// Pixel read mask, 0x205-0x208
	raddr[0] = 0x05;
	raddr[4] = 0x02;
		raddr[8] = 0xFF;	// All bits enabled
	raddr[0] = 0x06;
	raddr[4] = 0x02;
		raddr[8] = 0xFF;	// All bits enabled
	raddr[0] = 0x07;
	raddr[4] = 0x02;
		raddr[8] = 0xFF;	// All bits enabled
	raddr[0] = 0x08;
	raddr[4] = 0x02;
		raddr[8] = 0xFF;	// All bits enabled

	// Blink mask, 0x209-0x20c
	raddr[0] = 0x09;
	raddr[4] = 0x02;
		raddr[8] = 0x0;	// All bits disabled
	raddr[0] = 0x0a;
	raddr[4] = 0x02;
		raddr[8] = 0x0;	// All bits disabled
	raddr[0] = 0x0b;
	raddr[4] = 0x02;
		raddr[8] = 0x0;	// All bits disabled
	raddr[0] = 0x0c;
	raddr[4] = 0x02;
		raddr[8] = 0x0;	// All bits disabled

	// Tag table 0x300 - 0x30F. 24 bit word loaded w 3 writes
	for ( index = 0x300; index <= 0x30F; ++index )
	{
		raddr[0] = index & 0xFF;
		raddr[4] = (index & 0xFF00) >> 8;
			raddr[8] = 0x0;
			raddr[8] = 0x1;
			raddr[8] = 0x0;
	}
	
	// Load a ramp in the palatte. First 256 loactions
	for (index = 0; index < 256; index++)
	{
		raddr[0] = index & 0xFF;
		raddr[4] = (index & 0xFF00) >> 8;
		    raddr[0xC] = table[index];
		    raddr[0xC] = table[index];
		    raddr[0xC] = table[index];
	}	
	// Load a ramp in the palatte. Second 256 loactions
	for (index = 0x100; index < 0x200; index++)
	{
		raddr[0] = index & 0xFF;
		raddr[4] = (index & 0xFF00) >> 8;
		    raddr[0xC] = table[index - 0x100];
		    raddr[0xC] = table[index - 0x100];
		    raddr[0xC] = table[index - 0x100];
	}	
	// Last 4 locations 0x20C to 0x20F must be FF 
	for (index = 0x20C; index < 0x210; index++)
	{
		raddr[0] = index & 0xFF;
		raddr[4] = (index & 0xFF00) >> 8;
		    raddr[0xC] = 0xFF;
		    raddr[0xC] = 0xFF;
		    raddr[0xC] = 0xFF;
	}
	vm_deallocate( task_self(), Addr, size );	
	return;
}

UnBlank( int slot )
{
	vm_offset_t	Addr;		// Address of the board as mapped
	vm_size_t	size;
	kern_return_t	r;
	volatile unsigned int *raddr;
		
	r = ND_MapCSR( ND_port, debug_port, task_self(), slot, &Addr, &size );
	if ( r != KERN_SUCCESS )
	{
		mach_error( "ND_MapCSR", r );
		exit( 1 );
	}
	
	raddr = (volatile unsigned int *)(Addr + 0x2000);
	*raddr = 1;	// Unblank the screen
	
	vm_deallocate( task_self(), Addr, size );	
	return;
}

 static void
thud(sig)
    int sig;
{
	longjmp(Whoops, sig);
}

