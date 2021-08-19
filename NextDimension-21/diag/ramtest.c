#include <stdio.h>
#include <math.h>
#include <ctype.h>
#include <sys/ioctl.h>
#include "ndslot.h"
#include <sys/file.h>
#include <signal.h>
#include <setjmp.h>
#include <libc.h>

#define TEST_ADDR	1
#define TEST_ONES	2
#define TEST_ZEROS	4
#define TEST_RANDOM	8

#define DRAM_TEST	0x80000000
#define VRAM_ADDR	((int *)(((char *)Addr )+ 0x0E000000))
#define DRAM_ADDR	((int *)(((char *)Addr )+ 0x08000000))

#define DRAM_SIZE	0x800000

static char * Devices[] = 
{
	NULL,
	NULL,
	"/dev/slot2",
	NULL,
	"/dev/slot4",
	NULL,
	"/dev/slot6"
};
static volatile int *Addr;		// Address of the board as mapped
static int FileDesc;
static int Quiet = 0;

void	_NDMapBoard( int );
void	_NDFreeBoard( );
void	_NDVRAMtest( int, int, int, unsigned int, unsigned int, int );
static void usage( char * );
static void thud( int );
void InitErr();
void Err( char *, int, int, int );
void TellErr();

jmp_buf	Whoops;


main( argc, argv )
    int argc;
    char **argv;
{
    int slot;
    int tests;
    int mask;
    int runs;
    unsigned int startaddr, endaddr;
    char * program = argv[0];    
        
    slot = 2;
    tests = 0;
    mask = 0;
    startaddr = 0;
    endaddr = 0xFFFFFFFF;
    runs = 1;
    
    while (--argc)
    {
	if (*(*++argv) == '-')
	{
		if ( strcmp( "-dram", *argv ) == 0 )
		{
			tests |= DRAM_TEST;
			continue;
		}
		if ( strcmp( "-addr", *argv ) == 0 )
		{
			tests |= TEST_ADDR;
			continue;
		}
		else if ( strcmp( "-ones", *argv ) == 0 )
		{
			tests |= TEST_ONES;
			continue;
		}
		else if ( strcmp( "-zeros", *argv ) == 0 )
		{
			tests |= TEST_ZEROS;
			continue;
		}
		else if ( strcmp( "-rand", *argv ) == 0 )
		{
			tests |= TEST_RANDOM;
			continue;
		}
		else if ( strcmp( "-quiet", *argv ) == 0 )
		{
			Quiet = 1;
			continue;
		}
		else if ( strcmp( "-s", *argv ) == 0  && argc)
		{
			slot = atoi( *(++argv) );
			--argc;
			continue;
		}
		else if ( strcmp( "-runs", *argv ) == 0 && argc)
		{
			runs = atoi( *(++argv) );
			--argc;
			continue;
		}
		else if ( strcmp( "-mask", *argv ) == 0 && argc )
		{
			++argv;
			--argc;
			if ((*argv)[0] == '0' && ((*argv)[1]=='x' || (*argv)[1]=='X'))
				sscanf( &(*argv)[2], "%x", &mask );
			else
				sscanf( *argv, "%x", &mask );
			continue;
		}
		else if ( strcmp( "-start", *argv ) == 0 && argc )
		{
			++argv;
			--argc;
			if ((*argv)[0] == '0' && ((*argv)[1]=='x' || (*argv)[1]=='X'))
				sscanf( &(*argv)[2], "%x", &startaddr );
			else
				sscanf( *argv, "%x", &startaddr );
			continue;
		}
		else if ( strcmp( "-end", *argv ) == 0 && argc )
		{
			++argv;
			--argc;
			if ((*argv)[0] == '0' && ((*argv)[1]=='x' || (*argv)[1]=='X'))
				sscanf( &(*argv)[2], "%x", &endaddr );
			else
				sscanf( *argv, "%x", &endaddr );
			continue;
		}
		else
		{
		    fprintf( stderr, "Unknown option %s\n", *argv );
		    usage( program );
		    exit( 1 );
		}
	}
	else
		usage( program );
    }
    if ( (tests & ~DRAM_TEST) == 0 )
    	tests |= TEST_ADDR | TEST_ONES | TEST_ZEROS | TEST_RANDOM;
    if ( mask == 0 )
    {
    	if ( tests & DRAM_TEST )
		mask = 0xFFFFFFFF;
	else
		mask = 0xFFFFFF00;
    }
    _NDVRAMtest( slot, tests, mask, startaddr, endaddr, runs );
    
    exit(0);
}

 static void
usage( char * prog )
{
    fprintf(stderr, "usage: %s ", prog);
    fprintf(stderr, "\t[-s slot]\n\t[-mask hex_mask]\n\t[-addr]\n\t[-ones]\n" );
    fprintf( stderr, "\t[-zeroes]\n\t[-rand]\n\t[-start hex_addr]\n" );
    fprintf( stderr, "\t[-end hex_addr]\n\t[-runs number]\n\t[-quiet]\n\t[-dram]\n");
    exit(1);
}


 void
_NDMapBoard(int slot )
{	
	if ( slot < 2 || slot > 6 || Devices[slot] == NULL )
	{
		printf( "-s %d: Value must be 2, 4, or 6." );
		exit( 1 );
	}
	if ( (FileDesc = open( Devices[slot], O_RDWR )) == -1 )
	{
		perror( Devices[slot] );
		exit(1);
	}
	if ( ioctl( FileDesc, SLOTIOCGADDR_CI, (void *) &Addr ) == -1 )
	{
		perror( "SLOTIOCGADDR_CI" );
		exit(1);
	}
	if ( ioctl( FileDesc, SLOTIOCSTOFWD, (void *) 0 ) == -1 )
	{
		perror( "SLOTIOCSTOFWD" );
		exit(1);
	}
	return;
}

 void
_NDFreeBoard()
{
	ioctl( FileDesc, SLOTIOCDISABLE, (void *) 0 );
	close( FileDesc );
}

 static void
thud(sig)
    int sig;
{
	longjmp(Whoops, sig);
}
//
// The following section is the VRAMtest-specific code here.
//
#define SIZE_Y	832
#define SIZE_X	1152


 void
_NDVRAMtest( int slot, int flags, int mask, unsigned int startaddr,
	unsigned int endaddr, int runs )
{
    void (*old_SIGBUS)();
    void (*old_SIGSEGV)();
    volatile int *FrameStore;
    int i;
    int data;
    int run;
    int shift, shifthi;
    
    if ( flags & DRAM_TEST )
    {
	if ( endaddr >= DRAM_SIZE )
	    endaddr = DRAM_SIZE;
	if ( startaddr >= DRAM_SIZE )
	    startaddr = 0;
    	shift = 0;
	shifthi = 23;
	// scale startaddr and endaddr from byte to longword indices
	startaddr /= sizeof(long);
	endaddr /= sizeof(long);
    }
    else
    {
	if ( endaddr > (SIZE_X * SIZE_Y) )
	    endaddr = (SIZE_X * SIZE_Y);
	if ( startaddr > (SIZE_X * SIZE_Y) )
	    startaddr = 0;
    	shift = 8;
	shifthi = 28;
	// Pixel addresses are already longword addresses, and don't need scaling
    }

    old_SIGBUS = signal( SIGBUS, thud );
    old_SIGSEGV = signal( SIGSEGV, thud );
    
    if ( setjmp(Whoops) != 0 )
    {
	    (void) signal( SIGBUS, old_SIGBUS );
	    (void) signal( SIGSEGV, old_SIGSEGV );
	    fprintf( stderr, "RAMtest: hardware access error.\n" );
	    exit( 1 );
    }
    
    _NDMapBoard( slot );
    
    if ( flags & TEST_ADDR )
    {
    	fprintf( stderr,"Address Mapping Test\n" );
	for( run = 0; run < runs; ++run )
	{
	    FrameStore = ((flags & DRAM_TEST) ? DRAM_ADDR : VRAM_ADDR) + startaddr;
	    for ( i = startaddr; i < endaddr; ++i )
		*FrameStore++ = (((i<<shifthi)|(i << shift)) & mask);
	    FrameStore = ((flags & DRAM_TEST) ? DRAM_ADDR : VRAM_ADDR) + startaddr;
	    for ( i = startaddr; i < endaddr; ++i, ++FrameStore )
	    {
		data = (*FrameStore & mask);
		if ( data != (((i<<shifthi)|(i << shift)) & mask) )
		    Err( "Address Mapping",
		    		(int)FrameStore, data,
				((i<<shifthi)|(i << shift)) & mask );
	    }
	}
    }
    if ( flags & TEST_ONES )
    {
    	int cnt;
	int pattern;
    	fprintf( stderr, "Walking Ones Test\n" );
	for( run = 0; run < runs; ++run )
	{
	    pattern = 0x11111111;
	    for ( cnt = 0; cnt < 4; ++cnt )
	    {
	    	FrameStore = ((flags & DRAM_TEST) ? DRAM_ADDR : VRAM_ADDR) + startaddr;
		for ( i = startaddr; i < endaddr; ++i )
		    *FrameStore++ = (pattern & mask);
	   	FrameStore = ((flags & DRAM_TEST) ? DRAM_ADDR : VRAM_ADDR) + startaddr;
		for ( i = startaddr; i < endaddr; ++i, ++FrameStore )
		{
		    data = (*FrameStore & mask);
		    if ( data != (pattern & mask) )
			Err( "Walking Ones", (int)FrameStore, data, pattern & mask );
		}
		pattern <<= 1;
	    }
	}
    }
    if ( flags & TEST_ZEROS )
    {
    	int cnt;
	int pattern;
    	fprintf( stderr, "Walking Zeros Test\n" );
	for( run = 0; run < runs; ++run )
	{
	    pattern = 0xEEEEEEEE;

	    for ( cnt = 0; cnt < 4; ++cnt )
	    {
	    	FrameStore = ((flags & DRAM_TEST) ? DRAM_ADDR : VRAM_ADDR) + startaddr;
		for ( i = startaddr; i < endaddr; ++i )
		    *FrameStore++ = (pattern & mask);
	    	FrameStore = ((flags & DRAM_TEST) ? DRAM_ADDR : VRAM_ADDR) + startaddr;
		for ( i = startaddr; i < endaddr; ++i, ++FrameStore )
		{
		    data = (*FrameStore & mask);
		    if ( data != (pattern & mask) )
			Err( "Walking Zeros", (int)FrameStore, data, pattern & mask );
		}
		pattern <<= 1;
		pattern |= 1;	// Fold in another 1 to chase the 0
	    }
	}
    }
    if ( flags & TEST_RANDOM )
    {
    	int wr;
	
    	fprintf( stderr, "Random Numbers Test\n" );
	for( run = 0; run < runs; ++run )
	{
	    srandom(1);
	    FrameStore = ((flags & DRAM_TEST) ? DRAM_ADDR : VRAM_ADDR) + startaddr;
	    for ( i = startaddr; i < endaddr; ++i )
		*FrameStore++ = ((random() << shift) & mask);
		
	    srandom(1);
	    FrameStore = ((flags & DRAM_TEST) ? DRAM_ADDR : VRAM_ADDR) + startaddr;
	    for ( i = startaddr; i < endaddr; ++i, ++FrameStore )
	    {
		wr = (random() << shift) & mask;
		data = (*FrameStore & mask);
		if ( data != wr )
		    Err( "Random Numbers", (int)FrameStore, data, wr );
	    }
	}
    }
    TellErr();
    _NDFreeBoard();
    (void) signal( SIGBUS, old_SIGBUS );
    (void) signal( SIGSEGV, old_SIGSEGV );
}

static int Histogram[8];
static char *Titles[] = 
{
	"red high",
	"red low",
	"green high",
	"green low",
	"blue high",
	"blue low",
	"alpha high",
	"alpha low"
};

static int TotalErrs;

 void
InitErr()
{
	int i;
	for ( i = 0; i < ( sizeof Histogram / sizeof Histogram[0]); ++i )
		Histogram[i] = 0;
	TotalErrs = 0;
}

 void
Err( char *title, int addr, int rd, int wr )
{
	int i;
	unsigned errbits = rd ^ wr;
	
	if ( ! Quiet )
	{
	    fprintf( stderr,
		    "%s: 0x%08x: read 0x%08x, wrote 0x%08x [%08x]\n",
		    title, addr, rd, wr, errbits );
	}
		
	++TotalErrs;
	
	for ( i = 0; i < ( sizeof Histogram / sizeof Histogram[0]); ++i )
	{
		if ( errbits & 0xF0000000 )
			++Histogram[i];
		errbits <<= 4;
	}
}
 
 void
TellErr()
{
	int i;
	
	if ( ! TotalErrs )
	{
		fprintf( stderr, "Tests completed, no errors found.\n" );
		return;
	}
	fprintf( stderr, "Total of %d errors found:\n", TotalErrs );
	for ( i = 0; i < ( sizeof Histogram / sizeof Histogram[0]); ++i )
	{
		if ( Histogram[i] )
			fprintf( stderr, "\t%-12s %d\n", Titles[i], Histogram[i] );
	}
}
