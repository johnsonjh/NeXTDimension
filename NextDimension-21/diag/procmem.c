#include <stdio.h>
#include <ctype.h>
#include <mach_init.h>
#include <ND/NDlib.h>
#include <sys/file.h>
#include <signal.h>
#include <setjmp.h>
#include <libc.h>

/*
 *	procmem:
 *
 *	Test the processor memory interface, using 1, 2, 4, and 8 byte wide
 *	read and write operations.  Options permit setting the size of the 
 *	read or write operation, whether a read or a write is to be done, and
 *	the starting and ending addresses to be covered.
 *	
 *	The test program resides in the bottom 64 Kbytes of memory.
 *	Communications are done using the last few 32 bit words of
 *	memory in the 64 K section.  We make the optimistic assumption
 *	that these locations work!
 *
 *	0x4800FFE8	0 or bad address during read tests
 *	0x4800FFEC	bad data (as a 32 bit word) during read tests
 *	0x4800FFF0	high word of long bad data (as a 32 bit word) during read tests
 *	0x4800FFF4	Test direction in high half, size in low half
 *	0x4800FFF8	Start address
 *	0x4800FFFC	End Address
 */ 

#define TEST_BYTE	1
#define TEST_SHORT	2
#define TEST_WORD	4
#define TEST_LONG	8

#define TEST_READ	0x00010000
#define TEST_WRITE	0x00020000
#define TEST_FAST	0x00040000
#define TEST_AUTO	0x00080000
#define	SIZE_MASK	0xFFFF
#define DIR_MASK	0xFFFF0000
#define DIR_SHIFT	16

#define READ_TIMEOUT	10000
#define WRITE_TIMEOUT	20

#define MAX_ADDR	0x04000000
#define DEFAULT_MAX_ADDR	0x00400000	// 4 Mbytes
#define START_ADDR	0x00010000	// Code lives below 64 K
#define MASK_ADDR	0x07FFFFFF
#define I860_ADDR(s,a)	(((s)<<28)|0x08000000|((a)&MASK_ADDR))

#define BAD_ADDR_PORT(a)	((int *)(((char *)(a))+0x0000FFE8))
#define BAD_DATALOW_PORT(a)	((int *)(((char *)(a))+0x0000FFEC))
#define BAD_DATAHIGH_PORT(a)	((int *)(((char *)(a))+0x0000FFF0))
#define TESTINFO_PORT(a)	((int *)(((char *)(a))+0x0000FFF4))
#define START_ADDR_PORT(a)	((int *)(((char *)(a))+0x0000FFF8))
#define END_ADDR_PORT(a)	((int *)(((char *)(a))+0x0000FFFC))

static void usage( char * );
void procmem(int, int, int, int, int, int);
void readtest(int,volatile int *,int,int,int,int);
void writetest( int, volatile int *, int, int, int, int );
void autotest( int, volatile int *, int, int, int, int );
static void InitErr();
static void Err( char *, int, int, int, int );
static int TellErr();
static void halt860();
static void run860();
extern struct mach_header _mh_execute_header;
port_t ND_port;

ND_var_p handle = (ND_var_p) 0;

static jmp_buf	Whoops;
static int Quiet;
static int Cache = 0;

 static void
thud( int sig )
{
	longjmp(Whoops, sig);
}


main( argc, argv )
    int argc;
    char **argv;
{
    int slot = -1;
    int tests;
    int runs;
    int quiet;
    unsigned int startaddr, endaddr;
    char * program = argv[0];    
    int slotbits;
    int i;
    kern_return_t r;

    tests = 0;
    startaddr = ~0;
    endaddr = ~0;
    runs = 1;
    
    while (--argc)
    {
	if (*(*++argv) == '-')
	{

		if ( strcmp( "-quiet", *argv ) == 0 )
		{
			quiet = 1;
			continue;
		}
		else if ( strcmp( "-Quiet", *argv ) == 0 )
		{
			quiet = 2;
			continue;
		}
		else if ( strcmp( "-write", *argv ) == 0 )
		{
			tests |= TEST_WRITE;
			continue;
		}
		else if ( strcmp( "-auto", *argv ) == 0 )
		{
			tests |= TEST_AUTO;
			continue;
		}
		else if (strcmp( "-cache", *argv ) == 0 || strcmp( "-c", *argv ) == 0)
		{
			Cache = 1;
			continue;
		}
		else if ( strcmp( "-read", *argv ) == 0 )
		{
			tests |= TEST_READ;
			continue;
		}
		else if ( strcmp( "-byte", *argv ) == 0 )
		{
			tests |= TEST_BYTE;
			continue;
		}
		else if ( strcmp( "-short", *argv ) == 0 )
		{
			tests |= TEST_SHORT;
			continue;
		}
		else if ( strcmp( "-word", *argv ) == 0 )
		{
			tests |= TEST_WORD;
			continue;
		}
		else if ( strcmp( "-long", *argv ) == 0 )
		{
			tests |= TEST_LONG;
			continue;
		}
		else if ( strcmp( "-fast", *argv ) == 0 )
		{
			tests |= TEST_FAST;
			continue;
		}
		else if ( (strcmp("-s",*argv)==0 || strcmp("-slot",*argv)==0) && argc)
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
    // Load up default flags.
    if ( (tests & SIZE_MASK) == 0 ) 
    	tests |= (TEST_BYTE | TEST_SHORT | TEST_WORD | TEST_LONG);
    if ( (tests & DIR_MASK) == 0 )
    	tests |= (TEST_READ | TEST_WRITE | TEST_AUTO);
    if ( startaddr == ~0 )
	startaddr = START_ADDR;
    if ( endaddr == ~0 )
	endaddr = DEFAULT_MAX_ADDR;

    startaddr &= MASK_ADDR;
    endaddr &= MASK_ADDR;

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
    if ( (r = ND_Open( ND_port, slot )) != KERN_SUCCESS )
    	return r;

    handle = _ND_Handles_[SlotToHandleIndex(slot)];
    r = ND_MapDRAM( ND_port, handle->owner_port, task_self(), slot,
				&handle->dram_addr, &handle->dram_size );
    if ( r != KERN_SUCCESS )
    {
    	ND_Close( ND_port, slot );
	mach_error("ND_MapDRAM", r);
	exit(1);
    }

    procmem( slot, tests, startaddr, endaddr, runs, quiet );
    
    exit(0);
}

 static void
usage( char * prog )
{
    fprintf(stderr, "usage: %s ", prog);
    fprintf(stderr, "\t[-s slot]\n\t[-read]\n\t[-write]\n\t[-byte]\n" );
    fprintf( stderr, "\t[-short]\n\t[-word]\n\t[-long]\n\t[-start hex_addr]\n" );
    fprintf( stderr, "\t[-end hex_addr]\n\t[-runs number]\n\t[-quiet]\n");
    exit(1);
}

//
// Hit the RESET bit until the i860 is is CS8 mode, indicating it has gone 
// back to the ROM.  This may be bogus, depending on how the Memory Controller
// sets up the CS8 bit.
//
 static void
halt860()
{
    *ADDR_NDP_CSR0(handle) = (NDCSR_RESET860);
}

 static void
run860()
{
    volatile int *csr = (volatile int *)handle->csr_addr;
    int bits, wait;
    

    *ADDR_NDP_CSR0(handle) |= NDCSR_INT860ENABLE;
    // Post a bogus host interrupt as the handshake
    *ADDR_NDP_CSR0(handle) |= NDCSR_INT860;
    if ( Cache ) {
	for(wait = 0; wait < 10; wait++);	// make sure the processor has started
    	*ADDR_NDP_CSR0(handle) |= NDCSR_KEN860;
    }
}

 void
procmem(int slot, int tests, int startaddr, int endaddr, int runs, int quiet)
{
    int *Addr = (int *)ADDR_DRAM(handle);
    void (*old_SIGBUS)();
    void (*old_SIGSEGV)();

    Quiet = quiet;
    InitErr();
    old_SIGBUS = signal( SIGBUS, thud );
    old_SIGSEGV = signal( SIGSEGV, thud );
    if ( setjmp(Whoops) != 0 )
    {
	    (void) signal( SIGBUS, old_SIGBUS );
	    (void) signal( SIGSEGV, old_SIGSEGV );
	    if ( ! Quiet )
	    	fprintf( stderr, "procmem: hardware access error.\n" );
	    TellErr();
	    return;
    }

    if ( tests & TEST_READ )
    {
    	if ( ! quiet )
		fprintf( stderr, "i860 Read Operations\n" );
    	readtest( slot, Addr, startaddr, endaddr, tests, runs );
    }
    if ( tests & TEST_WRITE )
    {
    	if ( ! quiet )
		fprintf( stderr, "i860 Write Operations\n" );
    	writetest( slot, Addr, startaddr, endaddr, tests, runs );
    }
    
    if ( tests & TEST_AUTO )
    {
    	if ( ! quiet )
		fprintf( stderr, "i860 W/R Operations\n" );
    	autotest( slot, Addr, startaddr, endaddr, tests, runs );
    }
    
    halt860();
    (void) signal( SIGBUS, old_SIGBUS );
    (void) signal( SIGSEGV, old_SIGSEGV );
    
    TellErr();
}

 void
readtest(int slot, volatile int *Addr, int startaddr, int endaddr, int tests, int runs)
{
    int run;
    int spincount;
    int procaddr;
    int procdata;
    int *start;
    int *end;
    int align;
    int option;
    
    if ( tests & TEST_BYTE )
	align = 31;
    if ( tests & TEST_SHORT )
	align = 15;
    if ( tests & (TEST_WORD | TEST_LONG) )
	align = 31;
    if ( (startaddr & align) != 0 )
    {
	fprintf( stderr, "start addr %08X rounded down to %d byte boundry\n",
		startaddr, align + 1);
	startaddr &= align;
    }
    if ( (endaddr & align) != 0 )
    {
	fprintf( stderr, "end addr %08X rounded up to %d byte boundry\n",
		endaddr, align + 1);
	endaddr += align;
	endaddr &= align;
    }
    start = (int *)(((char *)Addr)+startaddr);
    end = (int *)(((char *)Addr)+endaddr); 
    
    halt860();			// Halted, no interrupt, no cache
    if ( NDLoadCodeFromSect(handle,&_mh_execute_header,"__I860","__qreadtest") == -1 )
    {
	perror( "i860 code module" );
	return;
    }
    option = 0;
    
    if ( tests & TEST_BYTE )
    {
    	unsigned int *addr;
	if ( ! Quiet )
	    fprintf( stderr, "i860 Byte Read: %08X - %08X\n", startaddr, endaddr );
	// Processor is halted on entry
	for ( run = 0; run < runs; ++run )
	{
	    // Load the test pattern in.
	    procdata = I860_ADDR(slot,(int)startaddr);
	    for ( addr = (unsigned int *)start; addr < (unsigned int *)end; ++addr )
	    {
	    	*addr = ((procdata & 0xFF) << 24) | (((procdata + 1) & 0xFF) << 16)
			| (((procdata + 2) & 0xFF) << 8) | ((procdata + 3) & 0xFF);

		procdata += 4;
	    }

	    *TESTINFO_PORT(Addr) = (TEST_READ|TEST_BYTE);
	    *START_ADDR_PORT(Addr) = I860_ADDR(slot,(int)startaddr);
	    *END_ADDR_PORT(Addr) = I860_ADDR(slot, (int)endaddr);
	    *BAD_ADDR_PORT(Addr) = 0;
	    run860();	// Running, no interrupt
	    spincount = 0;
	    while( *TESTINFO_PORT(Addr) != 0 ) // i860 running test
	    {
	    	if ( spincount++ > READ_TIMEOUT )
		{
			fprintf( stderr, "Timeout waiting for i860\n" );
			return;
		}
		if (*BAD_ADDR_PORT(Addr)!=0 && (procaddr=*BAD_ADDR_PORT(Addr)) != 0)
		{
			spincount = 0;
			procdata = *BAD_DATALOW_PORT(Addr) & 0xFF;
			Err("i860 Byte Read", procaddr, procdata, procaddr & 0xFF, 1);
			*BAD_ADDR_PORT(Addr) = 0;
		}
		usleep(1000);
	    }
	    halt860();	// Run complete.  Halt processor while we set up next run.
	}
    }
    if ( tests & TEST_SHORT )
    {
    	unsigned int *addr;
	if ( ! Quiet )
	    fprintf( stderr, "i860 Short Read: %08X - %08X\n", startaddr, endaddr );
	// Processor is halted on entry.
	for ( run = 0; run < runs; ++run )
	{
	    // Load the test pattern in.
	    procdata = I860_ADDR(slot,(int)startaddr);
	    for (addr = (unsigned int *)start; addr < (unsigned int *)end; ++addr)
	    {
	    	*addr = ((procdata & 0xFFFF) << 16) | ((procdata + 2) & 0xFFFF);
		procdata += 4;
	    }
	    *TESTINFO_PORT(Addr) = (TEST_READ|TEST_SHORT);
	    *START_ADDR_PORT(Addr) = I860_ADDR(slot,(int)startaddr);
	    *END_ADDR_PORT(Addr) = I860_ADDR(slot, (int)endaddr);
	    *BAD_ADDR_PORT(Addr) = 0;
	    run860();	// Running, no interrupt
	    spincount = 0;
	    while( *TESTINFO_PORT(Addr) != 0 ) // i860 running test
	    {
	    	if ( spincount++ > READ_TIMEOUT )
		{
			fprintf( stderr, "Timeout waiting for i860\n" );
			return;
		}
		if (*BAD_ADDR_PORT(Addr)!=0 && (procaddr=*BAD_ADDR_PORT(Addr)) != 0)
		{
			spincount = 0;
			procdata = *BAD_DATALOW_PORT(Addr) & 0xFFFF;
			Err("i860 Short Read",procaddr,procdata,procaddr & 0xFFFF,2);
			*BAD_ADDR_PORT(Addr) = 0;
		}
		usleep(1000);
	    }
	    halt860();
	}
    }
    if ( tests & TEST_WORD )
    {
    	unsigned int *addr;
	if ( ! Quiet )
	    fprintf( stderr, "i860 Word Read: %08X - %08X\n", startaddr, endaddr );
	// Processor is halted on entry
	for ( run = 0; run < runs; ++run )
	{
	    // Load the test pattern in.
	    procdata = I860_ADDR(slot,(int)startaddr);
	    for ( addr = (unsigned int *)start; addr < (unsigned int *)end; ++addr )
	    {
	    	*addr = ((unsigned int)procdata);
		procdata += 4;
	    }
	    *TESTINFO_PORT(Addr) = (TEST_READ|TEST_WORD);
	    *START_ADDR_PORT(Addr) = I860_ADDR(slot,(int)startaddr);
	    *END_ADDR_PORT(Addr) = I860_ADDR(slot, (int)endaddr);
	    *BAD_ADDR_PORT(Addr) = 0;
	    run860();	// Running, no interrupt
	    spincount = 0;
	    while( *TESTINFO_PORT(Addr) != 0 ) // i860 running test
	    {
	    	if ( spincount++ > READ_TIMEOUT )
		{
			fprintf( stderr, "Timeout waiting for i860\n" );
			return;
		}
		if (*BAD_ADDR_PORT(Addr)!=0 && (procaddr=*BAD_ADDR_PORT(Addr)) != 0)
		{
			spincount = 0;
			procdata = *BAD_DATALOW_PORT(Addr);
			Err("i860 Word Read", procaddr, procdata, procaddr, 4);
			*BAD_ADDR_PORT(Addr) = 0;
		}
		usleep(1000);
	    }
	    halt860();
	}
    }
    if ( tests & TEST_LONG )
    {
    	unsigned int *addr;
	if ( ! Quiet )
	    fprintf( stderr, "i860 Long Read: %08X - %08X\n", startaddr, endaddr );
	// Processor is halted on entry
	for ( run = 0; run < runs; ++run )
	{
	    // Load the test pattern in.
	    procdata = I860_ADDR(slot,(int)startaddr);
	    for ( addr = (unsigned int *)start; addr < (unsigned int *)end; ++addr )
	    {
	    	*addr = ((unsigned int)procdata);
		procdata += 4;
	    }
	    *TESTINFO_PORT(Addr) = (TEST_READ|TEST_LONG);
	    *START_ADDR_PORT(Addr) = I860_ADDR(slot,(int)startaddr);
	    *END_ADDR_PORT(Addr) = I860_ADDR(slot, (int)endaddr);
	    *BAD_ADDR_PORT(Addr) = 0;
	    run860();	// Running, no interrupt
	    spincount = 0;
	    while( *TESTINFO_PORT(Addr) != 0 ) // i860 running test
	    {
	    	if ( spincount++ > READ_TIMEOUT )
		{
			fprintf( stderr, "Timeout waiting for i860\n" );
			return;
		}
		if (*BAD_ADDR_PORT(Addr)!=0 && (procaddr=*BAD_ADDR_PORT(Addr)) != 0)
		{
			spincount = 0;
			procdata = *BAD_DATALOW_PORT(Addr);
			if ( (procaddr & 4) )
			    Err("i860 Long Read (high)",procaddr,procdata,procaddr,4);
			else
			    Err("i860 Long Read (low)",procaddr,procdata,procaddr,4);
			*BAD_ADDR_PORT(Addr) = 0;
		}
		usleep(1000);
	    }
	    halt860();
	}
    }
}

 void
writetest(int slot, volatile int *Addr, int startaddr, int endaddr, int tests,int runs)
{
    int run;
    int spincount;
    int procaddr;
    int procdata;
    int i;
    unsigned int datum;
    int *start;
    int *end;
    int option;

    if ( tests & TEST_FAST )
	option = NDCSR_KEN860;
    else
    	option = 0;
	
    start = (int *)(((char *)Addr)+startaddr);
    end = (int *)(((char *)Addr)+endaddr); 
    
    halt860();
    if ( NDLoadCodeFromSect(handle,&_mh_execute_header,"__I860","__writetest") == -1 )
    {
    	perror( "i860 code module" );
	return;
    }
    if ( tests & TEST_BYTE )
    {
    	unsigned int *addr;
	if ( ! Quiet )
	    fprintf( stderr, "i860 Byte Write: %08X - %08X\n", startaddr, endaddr );
	// Processor is halted on entry
	for ( run = 0; run < runs; ++run )
	{
	    *TESTINFO_PORT(Addr) = (TEST_WRITE|TEST_BYTE);
	    *START_ADDR_PORT(Addr) = I860_ADDR(slot,(int)startaddr);
	    *END_ADDR_PORT(Addr) = I860_ADDR(slot, (int)endaddr);
	    *BAD_ADDR_PORT(Addr) = 0;
	    run860();	// Running, no interrupt
	    spincount = 0;
	    while( *TESTINFO_PORT(Addr) != 0 ) // i860 running test
	    {
	    	if ( spincount++ > WRITE_TIMEOUT )
		{
			fprintf( stderr, "Timeout waiting for i860\n" );
			return;
		}
		sleep( 1 );
	    }
	    halt860();
	    procdata = I860_ADDR(slot,(int)startaddr);
	    for ( addr = (unsigned int *)start; addr < (unsigned int *)end; ++addr )
	    {
	    	datum = *addr;
		i = (datum >> 24) & 0xFF;
	    	if ( i != ((unsigned char)procdata &0xFF) )
		{
		   Err("i860 Byte Write",(int)addr,i,procdata & 0xFF,1);
		}
		++procdata;
		i = (datum >> 16) & 0xFF;
	    	if ( i != ((unsigned char)procdata &0xFF) )
		{
		   Err("i860 Byte Write",((int)addr)+1,i,procdata & 0xFF,1);
		}
		++procdata;
		i = (datum >> 8) & 0xFF;
	    	if ( i != ((unsigned char)procdata &0xFF) )
		{
		   Err("i860 Byte Write",((int)addr)+2,i,procdata & 0xFF,1);
		}
		++procdata;
		i = datum & 0xFF;
	    	if ( i != ((unsigned char)procdata &0xFF) )
		{
		   Err("i860 Byte Write",((int)addr)+3,i,procdata & 0xFF,1);
		}
		++procdata;
	    }
	}
    }
    if ( tests & TEST_SHORT )
    {
    	unsigned int *addr;
	if ( ! Quiet )
	    fprintf( stderr, "i860 Short Write: %08X - %08X\n", startaddr, endaddr );
	// Processor is halted on entry
	for ( run = 0; run < runs; ++run )
	{
	    *TESTINFO_PORT(Addr) = (TEST_WRITE|TEST_SHORT);
	    *START_ADDR_PORT(Addr) = I860_ADDR(slot,(int)startaddr);
	    *END_ADDR_PORT(Addr) = I860_ADDR(slot, (int)endaddr);
	    *BAD_ADDR_PORT(Addr) = 0;
	    run860();	// Running, no interrupt
	    spincount = 0;
	    while( *TESTINFO_PORT(Addr) != 0 ) // i860 running test
	    {
	    	if ( spincount++ > WRITE_TIMEOUT )
		{
			fprintf( stderr, "Timeout waiting for i860\n" );
			return;
		}
		sleep( 1 );
	    }
	    halt860();
	    procdata = I860_ADDR(slot,(int)startaddr);
	    for( addr = (unsigned int *)start; addr < (unsigned int *)end; ++addr )
	    {
	    	datum = *addr;
		i = (datum >> 16) & 0xFFFF;
	    	if ( i != ((unsigned short)procdata &0xFFFF) )
		{
		   Err("i860 Short Write", (int)addr,i,procdata & 0xFFFF,2);
		}
		procdata += 2;
		i = datum & 0xFFFF;
	    	if ( i != ((unsigned short)procdata &0xFFFF) )
		{
		   Err("i860 Short Write", ((int)addr)+2,i,procdata & 0xFFFF,2);
		}
		procdata += 2;
	    }
	}
    }
    if ( tests & TEST_WORD )
    {
    	unsigned int *addr;
	if ( ! Quiet )
	    fprintf( stderr, "i860 Word Write: %08X - %08X\n", startaddr, endaddr );
	// Processor is halted on entry
	for ( run = 0; run < runs; ++run )
	{
	    *TESTINFO_PORT(Addr) = (TEST_WRITE|TEST_WORD);
	    *START_ADDR_PORT(Addr) = I860_ADDR(slot,(int)startaddr);
	    *END_ADDR_PORT(Addr) = I860_ADDR(slot, (int)endaddr);
	    *BAD_ADDR_PORT(Addr) = 0;
	    run860();	// Running, no interrupt
	    spincount = 0;
	    while( *TESTINFO_PORT(Addr) != 0 ) // i860 running test
	    {
	    	if ( spincount++ > WRITE_TIMEOUT )
		{
			fprintf( stderr, "Timeout waiting for i860\n" );
			return;
		}
		sleep( 1 );
	    }
	    halt860();
	    procdata = I860_ADDR(slot,(int)startaddr);
	    for ( addr = (unsigned int *)start; addr < (unsigned int *)end; ++addr )
	    {
	    	if ( (i = *addr) != (unsigned)procdata )
		{
		   Err("i860 Word Write",(int)addr,i,procdata,4);
		}
		procdata += 4;
	    }
	}
    }
    if ( tests & TEST_LONG )
    {
    	unsigned int *addr;
	if ( ! Quiet )
	    fprintf( stderr, "i860 Long Write: %08X - %08X\n", startaddr, endaddr );
	// Processor is halted on entry
	for ( run = 0; run < runs; ++run )
	{
	    *TESTINFO_PORT(Addr) = (TEST_WRITE|TEST_LONG);
	    *START_ADDR_PORT(Addr) = I860_ADDR(slot,(int)startaddr);
	    *END_ADDR_PORT(Addr) = I860_ADDR(slot, (int)endaddr);
	    *BAD_ADDR_PORT(Addr) = 0;
	    run860();	// Running, no interrupt
	    spincount = 0;
	    while( *TESTINFO_PORT(Addr) != 0 ) // i860 running test
	    {
	    	if ( spincount++ > WRITE_TIMEOUT )
		{
			fprintf( stderr, "Timeout waiting for i860\n" );
			return;
		}
		sleep( 1 );
	    }
	    halt860();
	    procdata = I860_ADDR(slot,(int)startaddr);
	    for ( addr = (unsigned int *)start; addr < (unsigned int *)end; addr += 2 )
	    {
	    	if ( (i = *addr) != ((unsigned int)procdata) )
		{
		   Err("i860 Long Write (Low)",(int)addr,i,procdata,4);
		}
	    	if ( (i = *(addr + 1)) != ((unsigned int)procdata ^ ~0) )
		{
		   Err("i860 Long Write (High)",(int)(addr + 1),i,procdata ^ ~0,4);
		}
		procdata += 8;
	    }
	}
    }
}

 void
autotest(int slot, volatile int *Addr, int startaddr, int endaddr, int tests,int runs)
{
    int run;
    int spincount;
    int procaddr;
    int procdata;
    int *start;
    int *end;
    int align;
    int option;
    
    if ( tests & TEST_BYTE )
	align = 31;
    if ( tests & TEST_SHORT )
	align = 15;
    if ( tests & (TEST_WORD | TEST_LONG) )
	align = 31;
    if ( (startaddr & align) != 0 )
    {
	fprintf( stderr, "start addr %08X rounded down to %d byte boundry\n",
		startaddr, align + 1);
	startaddr &= align;
    }
    if ( (endaddr & align) != 0 )
    {
	fprintf( stderr, "end addr %08X rounded up to %d byte boundry\n",
		endaddr, align + 1);
	endaddr += align;
	endaddr &= align;
    }
    start = (int *)(((char *)Addr)+startaddr);
    end = (int *)(((char *)Addr)+endaddr); 
    
    halt860();			// Halted, no interrupt, no cache
    if ( NDLoadCodeFromSect(handle,&_mh_execute_header,"__I860","__autotest") == -1 )
    {
	perror( "i860 code module" );
	return;
    }
    option = 0;
    
    if ( tests & TEST_BYTE )
    {
    	unsigned int *addr;
	if ( ! Quiet )
	    fprintf( stderr, "i860 Byte W/R: %08X - %08X\n", startaddr, endaddr );
	// Processor is halted on entry
	for ( run = 0; run < runs; ++run )
	{

	    *TESTINFO_PORT(Addr) = (TEST_AUTO|TEST_BYTE);
	    *START_ADDR_PORT(Addr) = I860_ADDR(slot,(int)startaddr);
	    *END_ADDR_PORT(Addr) = I860_ADDR(slot, (int)endaddr);
	    *BAD_ADDR_PORT(Addr) = 0;
	    run860();	// Running, no interrupt
	    spincount = 0;
	    while( *TESTINFO_PORT(Addr) != 0 ) // i860 running test
	    {
	    	if ( spincount++ > READ_TIMEOUT )
		{
			fprintf( stderr, "Timeout waiting for i860\n" );
			return;
		}
		if (*BAD_ADDR_PORT(Addr)!=0 && (procaddr=*BAD_ADDR_PORT(Addr)) != 0)
		{
			spincount = 0;
			procdata = *BAD_DATALOW_PORT(Addr) & 0xFF;
			Err("i860 Byte W/R", procaddr, procdata, procaddr & 0xFF, 1);
			*BAD_ADDR_PORT(Addr) = 0;
		}
		usleep(1000);
	    }
	    halt860();	// Run complete.  Halt processor while we set up next run.
	}
    }
    if ( tests & TEST_SHORT )
    {
    	unsigned int *addr;
	if ( ! Quiet )
	    fprintf( stderr, "i860 Short W/R: %08X - %08X\n", startaddr, endaddr );
	// Processor is halted on entry.
	for ( run = 0; run < runs; ++run )
	{
	    *TESTINFO_PORT(Addr) = (TEST_AUTO|TEST_SHORT);
	    *START_ADDR_PORT(Addr) = I860_ADDR(slot,(int)startaddr);
	    *END_ADDR_PORT(Addr) = I860_ADDR(slot, (int)endaddr);
	    *BAD_ADDR_PORT(Addr) = 0;
	    run860();	// Running, no interrupt
	    spincount = 0;
	    while( *TESTINFO_PORT(Addr) != 0 ) // i860 running test
	    {
	    	if ( spincount++ > READ_TIMEOUT )
		{
			fprintf( stderr, "Timeout waiting for i860\n" );
			return;
		}
		if (*BAD_ADDR_PORT(Addr)!=0 && (procaddr=*BAD_ADDR_PORT(Addr)) != 0)
		{
			spincount = 0;
			procdata = *BAD_DATALOW_PORT(Addr) & 0xFFFF;
			Err("i860 Short W/R",procaddr,procdata,procaddr & 0xFFFF,2);
			*BAD_ADDR_PORT(Addr) = 0;
		}
		usleep(1000);
	    }
	    halt860();
	}
    }
    if ( tests & TEST_WORD )
    {
    	unsigned int *addr;
	if ( ! Quiet )
	    fprintf( stderr, "i860 Word W/R: %08X - %08X\n", startaddr, endaddr );
	// Processor is halted on entry
	for ( run = 0; run < runs; ++run )
	{
	    *TESTINFO_PORT(Addr) = (TEST_AUTO|TEST_WORD);
	    *START_ADDR_PORT(Addr) = I860_ADDR(slot,(int)startaddr);
	    *END_ADDR_PORT(Addr) = I860_ADDR(slot, (int)endaddr);
	    *BAD_ADDR_PORT(Addr) = 0;
	    run860();	// Running, no interrupt
	    spincount = 0;
	    while( *TESTINFO_PORT(Addr) != 0 ) // i860 running test
	    {
	    	if ( spincount++ > READ_TIMEOUT )
		{
			fprintf( stderr, "Timeout waiting for i860\n" );
			return;
		}
		if (*BAD_ADDR_PORT(Addr)!=0 && (procaddr=*BAD_ADDR_PORT(Addr)) != 0)
		{
			spincount = 0;
			procdata = *BAD_DATALOW_PORT(Addr);
			Err("i860 Word W/R", procaddr, procdata, procaddr, 4);
			*BAD_ADDR_PORT(Addr) = 0;
		}
		usleep(1000);
	    }
	    halt860();
	}
    }
    if ( tests & TEST_LONG )
    {
    	unsigned int *addr;
	if ( ! Quiet )
	    fprintf( stderr, "i860 Long W/R: %08X - %08X\n", startaddr, endaddr );
	// Processor is halted on entry
	for ( run = 0; run < runs; ++run )
	{
	    *TESTINFO_PORT(Addr) = (TEST_AUTO|TEST_LONG);
	    *START_ADDR_PORT(Addr) = I860_ADDR(slot,(int)startaddr);
	    *END_ADDR_PORT(Addr) = I860_ADDR(slot, (int)endaddr);
	    *BAD_ADDR_PORT(Addr) = 0;
	    run860();	// Running, no interrupt
	    spincount = 0;
	    while( *TESTINFO_PORT(Addr) != 0 ) // i860 running test
	    {
	    	if ( spincount++ > READ_TIMEOUT )
		{
			fprintf( stderr, "Timeout waiting for i860\n" );
			return;
		}
		if (*BAD_ADDR_PORT(Addr)!=0 && (procaddr=*BAD_ADDR_PORT(Addr)) != 0)
		{
			spincount = 0;
			procdata = *BAD_DATALOW_PORT(Addr);
			if ( (procaddr & 4) )
			    Err("i860 Long W/R (high)",procaddr,procdata,procaddr,4);
			else
			    Err("i860 Long W/R (low)",procaddr,procdata,procaddr,4);
			*BAD_ADDR_PORT(Addr) = 0;
		}
		usleep(1000);
	    }
	    halt860();
	}
    }
}


static int TotalErrs;

 static void
InitErr()
{
	TotalErrs = 0;
}

 static void
Err( char *title, int addr, int rd, int wr, int size )
{
	int i;
	unsigned errbits = rd ^ wr;
	size *= 2;	// bytes to nibbles in hex printout
	
	if ( ! Quiet )
	{
	    fprintf( stderr,
		    "%s: 0x%08x: read 0x%0*x, wrote 0x%0*x [%0*x]\n",
		    title, addr, size, rd, size, wr, size, errbits );
	}
	++TotalErrs;
}

 static int
TellErr()
{
	int i;
	
	if ( ! TotalErrs && ! Quiet )
	{
		fprintf( stderr, "Tests completed, no errors found.\n" );
		return 0;
	}
	if ( TotalErrs && Quiet < 2 )
	{
	    fprintf( stderr, "Total of %d errors found\n", TotalErrs );
	}
	return TotalErrs;
}

