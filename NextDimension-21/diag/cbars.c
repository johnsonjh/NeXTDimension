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

static volatile int *Addr;		// Address of the board as mapped
static int FileDesc;
void	_NDMapBoard( int );
void	_NDFreeBoard( );
void	_NDColorBars( int, double, int );
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
    int edge;
    double intensity;
    char * program = argv[0];
    void (*old_SIGBUS)();
    void (*old_SIGSEGV)();
    int slotbits;
    int i;
    kern_return_t r;
        
    intensity = 0.75;	// Reasonable defaults.
    edge = 0;
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
		case 'i':
		    if (*((*argv)+2))
			intensity = atof( (*argv)+2 );
		    else
			intensity = atof( *++argv ), --argc;
		    break;
		case 'e':
		    if (*((*argv)+2))
			edge = atoi( (*argv)+2 );
		    else
			edge = atoi( *++argv ), --argc;
		    break;

		default:
		    fprintf( stderr, "Unknown option %s\n", *argv );
		    usage( program );
		    exit( 1 );
		    break;
	    }
	}
	else
		usage( program );
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

    _NDColorBars( slot, intensity, edge );
    
    exit(0);
}

 static void
usage( char * prog )
{
	fprintf( stderr, "usage: %s ", prog );
	fprintf( stderr, "[-s slot] [-i intensity (0.0-1.0)] [-e edge]\n" );
	exit( 1 );
}

static vm_offset_t	_addr;		// Address of the board as mapped
static vm_size_t	_size;

 void
_NDMapBoard(int slot )
{	
	vm_offset_t	addr;		// Address of the board as mapped
	vm_size_t	size;
	kern_return_t	r;
	
	r = ND_MapFramebuffer(ND_port, debug_port, task_self(), slot, &_addr, &_size);
	if ( r != KERN_SUCCESS )
	{
		mach_error( "ND_MapFramebuffer", r );
		exit( 1 );
	}
	Addr = (volatile int *)_addr;
}

 void
_NDFreeBoard()
{
	vm_deallocate( task_self(), _addr, _size );
}

 static void
thud(sig)
    int sig;
{
	longjmp(Whoops, sig);
}
//
// The following section is the cbars-specific code here.
//
#define SIZE_Y	832
#define SIZE_X	1152

#define START_PAT1_Y	0
#define END_PAT1_Y	((int)(SIZE_Y * 0.8))
#define START_PAT2_Y	END_PAT1_Y
#define END_PAT2_Y	SIZE_Y

#define NCOLS		8
#define COL_WIDTH	(SIZE_X / NCOLS)

// Constants denoting the start and end of each column on the display
#define COL_0		0
#define COL_1		(COL_WIDTH)
#define COL_2		(COL_WIDTH * 2)
#define COL_3		(COL_WIDTH * 3)
#define COL_4		(COL_WIDTH * 4)
#define COL_5		(COL_WIDTH * 5)
#define COL_6		(COL_WIDTH * 6)
#define COL_7		(COL_WIDTH * 7)

#define GET_RED(i)	(((i)>>24) & 0xFF)
#define GET_GREEN(i)	(((i)>>16) & 0xFF)
#define GET_BLUE(i)	(((i)>>8) & 0xFF)

#define SET_RED(i, val)		(i) = ((i) & ~(0xFF<<24) | (((val)&0xFF)<<24))
#define SET_GREEN(i, val)	(i) = ((i) & ~(0xFF<<16) | (((val)&0xFF)<<16))
#define SET_BLUE(i, val)	(i) = ((i) & ~(0xFF<<8) | (((val)&0xFF)<<8))

#define	RED_COL		1
#define GREEN_COL	2
#define BLUE_COL	4

static int ColumnPattern1[NCOLS] =
{
	RED_COL | GREEN_COL | BLUE_COL,
	RED_COL | GREEN_COL,
	GREEN_COL | BLUE_COL,
	GREEN_COL,
	RED_COL | BLUE_COL,
	RED_COL,
	BLUE_COL,
	0
};
static int ColumnPattern2[NCOLS] =
{
	0,
	0,
	RED_COL | GREEN_COL | BLUE_COL,
	RED_COL | GREEN_COL | BLUE_COL,
	0,
	0,
	0,
	0
};

 void
_NDColorBars( int slot, double intensity, int edge )
{
    void (*old_SIGBUS)();
    void (*old_SIGSEGV)();
    volatile int *FrameStore;
    int		topscanbuf[SIZE_X];
    int		botscanbuf[SIZE_X];
    unsigned char columnbuf[2*COL_WIDTH];
    unsigned char * cp;
    int i;
    int nsamples;
    double interval;
    double coeff;
    int value;
    int col, startcol, endcol;
    int	maxPixel = (int)(255.0 * intensity);
   
    if ( maxPixel > 255 )
    	maxPixel = 255;
    if ( maxPixel < 0 )
    	maxPixel = 0;
    
    // Clear the buffers
    bzero( (char *) topscanbuf, sizeof topscanbuf );
    bzero( (char *) botscanbuf, sizeof botscanbuf );
    bzero( (char *) columnbuf, sizeof columnbuf );
    
    // Compute the template column buffer, including the cosine rolloff edges
    if ( edge >= COL_WIDTH )
    	edge = COL_WIDTH;
    if ( edge != 0 )
    {
    	interval = M_PI / (double)(edge + 1);
	coeff = M_PI - interval;
	for ( nsamples = 0; nsamples < edge; ++nsamples )
	{
		columnbuf[nsamples] = (int)((double)maxPixel * (cos(coeff)+1.0)/2.0);
		columnbuf[(COL_WIDTH - 1) + edge - nsamples] = columnbuf[nsamples];
		coeff -= interval;
	}
    }
    // fill in the span between the edges with maxPixel
    for ( i = edge; i < COL_WIDTH; ++i )
    	columnbuf[i] = maxPixel;
    // We now have a template for one column plus it's edges
    // Proceed to fill in a scanline template following the column template
    for ( col = 0; col < NCOLS; ++col )
    {
    	startcol =  (col == 0) ? 0 : (COL_WIDTH * col) - (edge / 2);
	endcol = (col == NCOLS - 1) ? SIZE_X : (COL_WIDTH * (col + 1)) + ((edge+1)/2);
	cp = (col == 0) ? &columnbuf[ edge / 2 ] : columnbuf;
	
	for ( i = startcol; i < endcol; ++i, ++cp )
	{
		if ( ColumnPattern1[col] & RED_COL )
		{
			value = GET_RED(topscanbuf[i]);
			value += *cp;
			if ( value > maxPixel )
				value = maxPixel;
			SET_RED(topscanbuf[i], value);
		}
		if ( ColumnPattern1[col] & GREEN_COL )
		{
			value = GET_GREEN(topscanbuf[i]);
			value += *cp;
			if ( value > maxPixel )
				value = maxPixel;
			SET_GREEN(topscanbuf[i], value);
		}
		if ( ColumnPattern1[col] & BLUE_COL )
		{
			value = GET_BLUE(topscanbuf[i]);
			value += *cp;
			if ( value > maxPixel )
				value = maxPixel;
			SET_BLUE(topscanbuf[i], value);
		}
	}
    }
    // Fill in the pattern for the bottom portion of the screen
    for ( col = 0; col < NCOLS; ++col )
    {
    	startcol =  (col == 0) ? 0 : (COL_WIDTH * col) - (edge / 2);
	endcol = (col == NCOLS - 1) ? SIZE_X : (COL_WIDTH * (col + 1)) + ((edge+1)/2);
	cp = (col == 0) ? &columnbuf[ edge / 2 ] : columnbuf;
	
	for ( i = startcol; i < endcol; ++i, ++cp )
	{
		if ( ColumnPattern2[col] & RED_COL )
		{
			value = GET_RED(botscanbuf[i]);
			value += *cp;
			if ( value > maxPixel )
				value = maxPixel;
			SET_RED(botscanbuf[i], value);
		}
		if ( ColumnPattern2[col] & GREEN_COL )
		{
			value = GET_GREEN(botscanbuf[i]);
			value += *cp;
			if ( value > maxPixel )
				value = maxPixel;
			SET_GREEN(botscanbuf[i], value);
		}
		if ( ColumnPattern2[col] & BLUE_COL )
		{
			value = GET_BLUE(botscanbuf[i]);
			value += *cp;
			if ( value > maxPixel )
				value = maxPixel;
			SET_BLUE(botscanbuf[i], value);
		}
	}
    }
    old_SIGBUS = signal( SIGBUS, thud );
    old_SIGSEGV = signal( SIGSEGV, thud );
    
    if ( setjmp(Whoops) != 0 )
    {
	    (void) signal( SIGBUS, old_SIGBUS );
	    (void) signal( SIGSEGV, old_SIGSEGV );
	    fprintf( stderr, "cbars: hardware access error.\n" );
	    exit( 1 );
    }
    
    _NDMapBoard( slot );
    FrameStore = Addr + (START_PAT1_Y * SIZE_X);
    for ( i = START_PAT1_Y; i < END_PAT1_Y; ++i )
    {
    	bcopy( (char *) topscanbuf, (char *)FrameStore, sizeof topscanbuf );
	FrameStore += SIZE_X;
    }
    FrameStore = Addr + (START_PAT2_Y * SIZE_X);
    for ( i = START_PAT2_Y; i < END_PAT2_Y; ++i )
    {
    	bcopy( (char *) botscanbuf, (char *)FrameStore, sizeof botscanbuf );
	FrameStore += SIZE_X;
    }
    _NDFreeBoard();
    (void) signal( SIGBUS, old_SIGBUS );
    (void) signal( SIGSEGV, old_SIGSEGV );
}
