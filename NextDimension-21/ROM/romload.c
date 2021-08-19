#include "ND/ND_thread_status.h"	/* MUST BE FIRST! */
#include "ND/ND_machine.h"		/* MUST BE SECOND! */
#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <mach.h>
#include <sys/errno.h>
#include <sys/loader.h>

/* #define DEBUG	1 */
/* #define TEST 1 */

char *progname;
char hex[] = "0123456789ABCDEF";
unsigned maxdata = 64/2;

char *atob();
void srec();
void error();
void warning();
extern void * malloc();

int MapProgram( char * );

static void * LoadImage = (void *) 0;

int MapPcode( char *, int * );

static void * PcodeImage = (void *) 0;
static int PcodeLen = 0;

static char * ROMaddr;	/* Destination address for ROM image */
static int ROMlength = 128 * 1024;

#define ROM_BASE		0xFFFE0000
#define ROM_RESET_VEC		0xFFFFFF00
#define ROM_PCODE_ADDR_MSB	0xFFFFFFB8
#define ROM_PCODE_ADDR_B2	0xFFFFFFC0
#define ROM_PCODE_ADDR_B1	0xFFFFFFC8
#define ROM_PCODE_ADDR_LSB	0xFFFFFFD0
#define ROM_BYTE_LANE_ID	0xFFFFFFD8
#define ROM_SIGNATURE		0xFFFFFFE0
#define ROM_NBIC_ID_MSB		0xFFFFFFF0
#define ROM_NBIC_ID_B2		0xFFFFFFF4
#define ROM_NBIC_ID_B1		0xFFFFFFF8
#define ROM_NBIC_ID_LSB		0xFFFFFFFC

#define OUR_ROM_SIGNATURE	0xA5
#define OUR_NBIC_ID		0xC0000001

#define ROM_OFF(item)		((item)-ROM_BASE)
#define ADDR(item)		(ROMaddr + ROM_OFF(item))

/* IDs for the ROM we know how to program. */
#define MFG_ID	0x89
#define DEV_ID	0xb4


unsigned int	ByteLaneID = 4;
unsigned int	ROM_EndProgramSpace;	/* Last avail addr for ROM program */
unsigned int	EntryPoint = ROM_BASE;


struct thread_aout
{
	struct thread_command ut;
	unsigned long flavor;
	unsigned long count;
	struct NextDimension_thread_state_regs cpu;
};

void
usage()
{
	error(	"Usage: %s [-(2|3|4)] [-b(1|4|8)] "
		"[-s(2|4|6)] [-S] [-p PCODE] [OBJ]",
		progname);
}

main(argc, argv)
int argc;
char **argv;
{
	int address_len = 4; /* address length in bytes */
	int slot = -1;
	int Srecs = 0;
	char *objfile = (char *)NULL;
	char *pcodefile = (char *)NULL;
	int size;

	/*
	 * romimage -[2|3|4] [-BbytelaneID] [-p pcode] [OBJ]
	 *
	 *   Converts OBJ to Moto S-record format.
	 *
	 *   -a specifies s-record address length.  Addresses can
	 *   be either 2, 3, or 4 bytes long.  A warning is given
	 *   if OBJ addresses won't fit in the given address size.
	 *
	 *   If -T is specified, srec addresses for text
	 *   start at addr rather than the a.out entry point,
	 *   If addr is not specified, 0 is used.
	 *
	 *   If -D is given, the data is placed at addr rather
	 *   than immediately after the text,  if addr
	 *   is not specified, 0 is used.  A warning is
	 *   printed if text and data overlap.
	 */

	progname = *argv++;
	argc--;

	for (; argc && **argv == '-'; argv++, argc--) {

		switch ((*argv)[1]) {

		case '2':
			address_len = 2;
			break;

		case '3':
			address_len = 3;
			break;

		case '4':
			address_len = 4;
			break;

		case 'p':
			if ( argc > 1 )
			{
				++argv;
				--argc;
				pcodefile = *argv;	
			}
			else
			    usage();
			
		case 'b':
			if (*atob(&(*argv)[2], &ByteLaneID, 16))
				usage();
			switch( ByteLaneID )
			{
				case 1:
				case 4:
				case 8:
					break;
				default:
					usage();
					ByteLaneID = 4;
					break;
			}
		case 's':
			if (*atob(&(*argv)[2], &slot, 16))
				usage();
			switch( slot )
			{
				case 2:
				case 4:
				case 6:
					break;
				default:
					usage();
					slot = -1;
					break;
			}
			break;

		case 'S':
			Srecs = 1;
			break;

 		default:
			usage();
		}
	}
	if ( argc )
		objfile = *argv;

	//
	// Map the program file into memory, so we can massage the bits
	// as needed.  Mostly, this means that we will munge the text into
	// the weird byte and word ordering we need.
	//
	if ( objfile != (char *)NULL )
	{
	    if ( MapProgram( objfile ) == -1 )
		    error("Can't open %s", objfile);
	}
	else
	{
	    LoadImage = (void*)getsectdata("__i860", "__bootcode", &size);
	    if ( LoadImage == (void *) 0 )
		    error( "Can't find seg __i860, sect __bootcode" );
	}

	if ( pcodefile != (char *)NULL )
	{
	    if ( MapPcode( pcodefile, &PcodeLen ) == -1 )
		    error("Can't open %s", pcodefile);
	}
	else
	{
	    PcodeImage = (void*)getsectdata("__i860", "__pcode", &PcodeLen);
	    if ( LoadImage == (void *) 0 )
		    error( "Can't find seg __i860, sect __pcode" );
	}
	
	ROM_EndProgramSpace = ROM_RESET_VEC - (PcodeLen * ByteLaneID);

	ROMaddr = (char *) malloc( ROMlength );
	if ( ROMaddr == (char *) 0 )
	{
		perror( "malloc" );
		exit( 1 );
	}
	bzero( ROMaddr, ROMlength );
	
	/* Ready to roll. Build the ROM image */
	if ( BuildImage() == -1 )
	{
		perror( *argv );
		exit( 1 );
	}
	
	if ( Srecs )
	{
	    /*
	     * Dump the ROM image in EXORMAX S-record format.
	     *
	     * Something to fill the sign-on srec
	     */
	    hdrsrec(address_len, 0, "Copyright 1990 NeXT Computer, Inc.");
	    srec( ROMaddr, address_len, ROMlength, ROM_BASE, *argv);
	    eofsrec(address_len, ROM_BASE);
	}
	else
	{
		if ( ! ProgramROM( slot, ROMaddr, ROMlength, ROM_BASE ) )
			exit( 1 );
	}
	
	exit(0);
}

BuildImage()
{
	if ( BuildProgram() == -1 )	// Load the ROM program into the image.
		return -1;
	BuildPcode();		// Load the P-code into the image
	BuildBootVector();	// Load the i860 boot vector code
	BuildID();		// Load the ID stuff into the image
	BuildCheckSum();	// Check sum to go in 1st 4 bytes of ROM
}

BuildProgram()
{
    struct mach_header *mhp;
    struct load_command *lcmd;
    struct segment_command *scmd;
    struct thread_aout *thread = (struct thread_aout *) NULL;
    char *data;
    char *isrc;
    char *idest;
    int ncmds;
    int cnt;
    int txtstart;

    mhp = (struct mach_header *) LoadImage;
    if ( mhp->magic != MH_MAGIC )
    {
    	errno = ENOEXEC;
    	return -1;
    }
    if ( mhp->cputype != CPU_TYPE_I860 || mhp->cpusubtype != CPU_SUBTYPE_BIG_ENDIAN )
    {
    	errno = ENOEXEC;
    	return -1;
    }
    if ( (mhp->flags & MH_NOUNDEFS) == 0 )
    {
    	errno = ENOEXEC;
    	return -1;
    }
    if ( mhp->filetype != MH_PRELOAD )
    {
    	errno = ENOEXEC;
    	return -1;
    }
    // Load code and data into the DRAM
    ncmds = mhp->ncmds;
    data = (char *) LoadImage;
    data += sizeof (struct mach_header);	// Skip over header to load cmds
    while ( ncmds-- )
    {
	lcmd = (struct load_command *)data;
	if ( lcmd->cmd != LC_SEGMENT )
	{
		if ( lcmd->cmd == LC_THREAD || lcmd->cmd == LC_UNIXTHREAD )
			thread = (struct thread_aout *)data;
		data += lcmd->cmdsize;
		continue;
	}
	// load actions here
	scmd = (struct segment_command *) data;
	isrc = ((char *)LoadImage) + scmd->fileoff;
	idest = ((char *)ROMaddr) + (scmd->vmaddr - ROM_BASE);
	if ( scmd->filesize == 0 )	// Nothing to load in this seg!
	{
		data += lcmd->cmdsize;	// Move to the next command
		continue;
	}
	// Check for out of bounds addresses
	if ( strcmp( scmd->segname, "__PAGEZERO" ) != 0 ) // Ignore __PAGEZERO
	{
	    if ( scmd->vmaddr < ROM_BASE
	    	|| scmd->vmaddr >= ROM_EndProgramSpace
	    	|| (scmd->vmaddr + scmd->vmsize) < ROM_BASE
	    	|| (scmd->vmaddr + scmd->vmsize) >= ROM_EndProgramSpace )
	    {
		warning("address lies outside of ROM");
		data += lcmd->cmdsize;	// Move to the next command
		continue;
	    }
	}
#if defined(TEST)
    printf( "Seg %s, fileoff 0x%x, filesize 0x%x, vmaddr 0x%x, vmsize 0x%x\n",
    		scmd->segname, scmd->fileoff, scmd->filesize,
		scmd->vmaddr, scmd->vmsize );
#endif
	if ( strcmp( scmd->segname, "__TEXT" ) == 0 )
	{
	    if ( scmd->flags & SG_HIGHVM )
	    {
	    	idest += (scmd->vmsize - scmd->filesize);
		// Copy the portion of this segment in file to ROM image
#if defined(TEST)
		printf( "HIGHVM: Write insns to 0x%x\n", ROM_BASE + (idest - ROMaddr));
#endif
		MungeText( isrc, idest, scmd->filesize );
	    }
	    else
	    {
		// Copy the portion of this segment in file to ROM image
#if defined(TEST)
		printf( "Write insns to 0x%x\n", ROM_BASE + (idest - ROMaddr));
#endif
		MungeText( isrc, idest, scmd->filesize );
	    }
	}
	else if ( strcmp( scmd->segname, "__PAGEZERO" ) != 0 ) // Ignore __PAGEZERO
	{	// Data, Objective C, strings and such, all big-endian data
	    if ( scmd->flags & SG_HIGHVM )
	    {
		// Skip the portion of core not covered by the file.
		idest += (scmd->vmsize - scmd->filesize);
		// Copy the portion of this segment in file to core
#if defined(TEST)
		printf( "HIGHVM: Write data to 0x%x\n", ROM_BASE + (idest - ROMaddr));
#endif
		bcopy( isrc, idest, scmd->filesize );
	    }
	    else
	    {
		// Copy the portion of this segment in file to core
#if defined(TEST)
		printf( "Write data to 0x%x\n", ROM_BASE + (idest - ROMaddr));
#endif
		bcopy( isrc, idest, scmd->filesize );
	    }
	}
	data += lcmd->cmdsize;	// Move to the next command
    }
    // Check for a valid entry point
    if ( thread == (struct thread_aout *)0 ||
    	 thread->flavor != NextDimension_THREAD_STATE_REGS )
    {
    	errno = ENOEXEC;
    	return -1;
    }
    EntryPoint = thread->cpu.pc;
#if defined(TEST)
    printf( "Entry point 0x%08x\n", thread->cpu.pc );
#endif

}

BuildPcode()
{
	unsigned char *dst;
	
	dst = (unsigned char *)ROMaddr + (ROM_EndProgramSpace - ROM_BASE);
	MungePcode( (unsigned char *)PcodeImage, dst, PcodeLen, ByteLaneID );
	return 0;
}

//
// Build the reset vector code for the ROM Image
// This code consists of 2 NOPs (for a chip bug), followed by
// a branch to the ROM entry point, and several more NOPs.
//
#define OP_NOP		0xA0000000	// big-endian NOP
#define OP_BR_RAW	0x68000000	// Unswapped branch opcode
#define TRAP_VECTOR	0xFFFFFF00	// Address receiving control on a trap
#define OPMASK		0xFC000000	// Mask for branch opcode bits
	
BuildBootVector()
{
	unsigned long br_op;
	unsigned long tmp;
	unsigned char *buf;
	unsigned long code[9];	// Buffer to hold 9 insns
	
	// Compute branch relative address, in a form suitable for a br opcode.
	tmp = EntryPoint - ((unsigned long) ROM_RESET_VEC); // Get relative span
	tmp >>= 2;					    // Convert to word addr
	tmp += -3;	// Adjust for 2 NOPs, + 1 for pc increment
		    // NOPs are here for a chip bug.
	tmp &= ~OPMASK;					// Mask out opcode bits
	tmp |= OP_BR_RAW;				// OR in the branch opcode
	br_op = tmp;
    
	code[0] = OP_NOP;			// nop
	code[1] = OP_NOP;			// nop
	code[2] = br_op;			// br EntryPoint
	code[3] = OP_NOP;			// nop
	code[4] = OP_NOP;			// nop
	code[5] = OP_NOP;			// nop
	code[6] = OP_NOP;			// nop
	code[7] = OP_NOP;			// nop
	code[8] = OP_NOP;			// nop
    
	// Load the code into the ROM image
	MungeText(	(unsigned char *)code,
		    ROMaddr+(ROM_RESET_VEC-ROM_BASE),
		    sizeof code);
	
	return 0;
}

BuildID()
{
	unsigned char *dst;
	unsigned char datum;
	
	/* Leave bits 24-27 as 0 for addition of Slot ID by the CPU ROM */
	SetID( ROM_PCODE_ADDR_MSB, (ROM_EndProgramSpace >> 24) & 0xF0 );
	SetID( ROM_PCODE_ADDR_B2, (ROM_EndProgramSpace >> 16) & 0xFF );
	SetID( ROM_PCODE_ADDR_B1, (ROM_EndProgramSpace >> 8) & 0xFF );
	SetID( ROM_PCODE_ADDR_LSB, ROM_EndProgramSpace & 0xFF );
	
	SetID( ROM_BYTE_LANE_ID, ByteLaneID );
	SetID( ROM_SIGNATURE, OUR_ROM_SIGNATURE );
	
	SetID( ROM_NBIC_ID_MSB, (OUR_NBIC_ID >> 24) & 0xFF );
	SetID( ROM_NBIC_ID_B2, (OUR_NBIC_ID >> 16) & 0xFF );
	SetID( ROM_NBIC_ID_B1, (OUR_NBIC_ID >> 8) & 0xFF );
	SetID( ROM_NBIC_ID_LSB, OUR_NBIC_ID & 0xFF );
	
}

SetID( addr, data )
    unsigned int addr;
    unsigned char data;
{
	unsigned char *dst;
	int i;
	
	dst = (unsigned char *)ROMaddr + (addr - ROM_BASE);
	for ( i = 0; i < 4; ++i )
		*dst++ = data;
}

//
// Checksum the entire ROM, except for the first 4 bytes.
// The first four bytes are then set to hold the checksum.
//
BuildCheckSum()
{
	unsigned int * ip;
	unsigned int sum;
	unsigned char *cp;
	int cnt;
	
	cp = (unsigned char *)ROMaddr + sizeof (sum);
	sum = 0;
	for ( cnt = sizeof (sum); cnt < ROMlength; ++cnt )
		sum += *cp++;
	
	ip = (unsigned int *)ROMaddr;
	*ip = sum;
}

 int MapProgram( char *file )
{
	int fd;
	struct stat stbuf;
	int retval;
	    
	// Prep text and data for loading
	// First, map it in so we can hand a pointer to the Mach
	// section/segment functions.
	
	if ( (fd = open( file, O_RDONLY, 0644 )) == -1 )
	    return -1;

	if ( fstat( fd, &stbuf ) == -1 )
	{
	    close( fd );
	    return -1;
	}
	
	if ( map_fd(fd, (vm_offset_t)0, (vm_offset_t *)&LoadImage,
		    1, (vm_size_t)stbuf.st_size ) != KERN_SUCCESS )
	{
	    close( fd );
	    return -1;
	}
	return 0;
}

 int MapPcode( char *file, int *len )
{
	int fd;
	struct stat stbuf;
	int retval;
	    
	// Prep text and data for loading
	// First, map it in so we can hand a pointer to the Mach
	// section/segment functions.
	
	if ( (fd = open( file, O_RDONLY, 0644 )) == -1 )
	    return -1;

	if ( fstat( fd, &stbuf ) == -1 )
	{
	    close( fd );
	    return -1;
	}
	*len = stbuf.st_size;
	if ( map_fd(fd, (vm_offset_t)0, (vm_offset_t *)&PcodeImage,
		    1, (vm_size_t)stbuf.st_size ) != KERN_SUCCESS )
	{
	    close( fd );
	    return -1;
	}
	return 0;
}

//
// The P-code bytes are replicated 4 times, over the width of a 32 bit word.
//
MungePcode( unsigned char *src, unsigned char *dst, int len, int lanes )
{
	int i, j;
	unsigned char *out;
	
	for ( i = 0; i < len; ++i )
	{
		for ( j = 0; j < lanes; ++j )
		    *dst++ = *src;
		++src;
	}
}
//
// The i860 text segment must be byte and word swapped before packing into the ROM.
// The text must be on a 64 bit boundry, and must be a multiple of 64 bits in length.
//
// The byte sequence to be written for each pair of words is as shown:
//	 3 2 1 0 7 6 5 4
//	|- - - -|- - - -|
MungeText( src, dst, data_len)
    unsigned char *src;
    unsigned char *dst;
    unsigned data_len;
{
	struct munge{
		unsigned char	byte[4];
	} munge;
	struct munge *msrc;
	struct munge *mdst;
	int i;
	unsigned char tmp;
	
	msrc = (struct munge *)src;
	mdst = (struct munge *)dst;
	while ( data_len )
	{
		for ( i = 0; i < 4; ++i )
			munge.byte[ 3 - i ] = msrc->byte[ i ];
		*mdst = munge;
		++msrc;
		++mdst;
		data_len -= 4;
	}
}

/*
 * Code to drive the programming of the NextDimension ROM.
 * This code connects to the NextDimension device driver
 * and directs the overall programming operation.  The bit fiddling
 * real time stuff is done within the Mach device driver.
 */
 int
ProgramROM( int slot, unsigned char *data, int length, unsigned long start)
{
    port_t ND_port;
    kern_return_t r;
    
    if ( ND_Load_MachDriver() != KERN_SUCCESS )
    {
    	mach_error( "Mach device driver", r );
	return 0;
    }
    r = netname_look_up(name_server_port, "", "NextDimension", &ND_port);
    if (r != KERN_SUCCESS)
    {
    	mach_error( "netname_look_up", r );
	return 0;
    }
    
    /* If no slot was specified, try all slots. */
    if ( slot == -1 )
    {
    	for ( slot = 2; slot <= 6; slot += 2 )
	{
	    program_slot(ND_port, slot, data, length, start);
	}
	return 1;
    }
    else
    {
    	return( program_slot(ND_port, slot, data, length, start) );
    }
}

program_slot(	port_t ND_port,
		int slot,
		unsigned char *data,
		int length,
		unsigned long start )
{
	kern_return_t r;
	int cnt;
	unsigned mfg, dev;
	unsigned sum, record_sum;

	if (ND_ROM_Identifier(ND_port, slot, &mfg, &dev ) != KERN_SUCCESS)
	    return 0;
	if ( mfg != MFG_ID || dev != DEV_ID )
	{
	    printf(	"Slot %d: Unknown device: "
		    "Mfg Code %02X, Dev Code %02X\n", slot, mfg, dev);
	    return 0;
	}
	printf( "Slot %d: Erasing EEROM... ", slot );
	fflush( stdout );
	if ( (r = ND_ROM_Erase( ND_port, slot )) != KERN_SUCCESS )
	{
	    printf( "Erase failed (%d).\n", r );
	    return 0;
	}
	printf( "Programming EEROM... " );
	fflush( stdout );
	while ( length )
	{
	    if ( length > 1024 )
		    cnt = 1024;
	    else
		    cnt = length;
	    r = ND_ROM_Program( ND_port, slot, start, data, cnt );
	    if ( r != KERN_SUCCESS )
	    {
		printf( "Program failed (%d).\n", r );
		printf( "Address range 0x%x to 0x%x\n", start, start+cnt-1 );
		return 0;
	    }
	    start += cnt;
	    data += cnt;
	    length -= cnt;				
	}
	printf("Check Sum ");
	fflush( stdout );
	r = ND_ROM_CheckSum(ND_port, slot, &sum, &record_sum);
	if( r != KERN_SUCCESS )
	{
	    printf("call failed (%d).\n",r);
	    fflush( stdout );
	    return 0;
	}
	if ( record_sum != sum )
	{
	    printf("Failed: %08X != recorded %08X\n",
		    sum, record_sum);
	    fflush( stdout );
	    return 0;
	}
	else
	    printf("%08X\n", sum);
	fflush( stdout );
	return 1;
}

hdrsrec(address_len, addr_field, str)
unsigned addr_field;
char *str;
{
	unsigned csum;
	int byte;
	char *cp;
#ifdef DEBUG
	int i;
#endif DEBUG
	unsigned hexint();

	csum = 0;

	putchar('S');
	/* record type */
	putchar(hex[0]);
#ifdef DEBUG
	putchar(' ');
#endif DEBUG
	/* record length (address_len + data_len + csum_byte) */
	csum += hexint(address_len + strlen(str) + 1, 1);
#ifdef DEBUG
	putchar(' ');
#endif DEBUG
	/* address */
	csum += hexint(addr_field, address_len);
#ifdef DEBUG
	putchar(' ');
	i = 0;
#endif DEBUG

	for (cp = str; *cp; cp++) {
		csum += hexint(*cp, 1);
#ifdef DEBUG
		if ((++i & 3) == 0)
			putchar(' ');
#endif DEBUG
	}
#ifdef DEBUG
	putchar(' ');
#endif DEBUG
	hexint(~csum, 1);
	putchar('\n');
}

void
srec(obj, address_len, data_len, data_base, filename)
void *obj;
int address_len;
unsigned data_len;
unsigned data_base;
char *filename;
{
	unsigned csum;
	unsigned len;
	int byte;
	unsigned char *bp = (unsigned char *)obj;
#ifdef DEBUG
	int i;
#endif DEBUG
	unsigned hexint();

	if (data_base + data_len < data_base && (data_base + data_len) != 0 )
		warning("address range exceeds 32 bits");
	while (data_len) {
		len = data_len > maxdata ? maxdata : data_len;
		data_len -= len;

		csum = 0;

		putchar('S');
		/* record type */
		putchar(hex[address_len-1]);
#ifdef DEBUG
		putchar(' ');
#endif DEBUG
		/* record length (address_len + data_len + csum_byte) */
		csum += hexint(address_len + len + 1, 1);
#ifdef DEBUG
		putchar(' ');
#endif DEBUG
		/* address */
		csum += hexint(data_base, address_len);
		data_base += len;
#ifdef DEBUG
		putchar(' ');
		i = 0;
#endif DEBUG
		while (len--) {
			byte = *bp++;
			csum += hexint(byte, 1);
#ifdef DEBUG
			if ((++i & 3) == 0)
				putchar(' ');
#endif DEBUG
		}
#ifdef DEBUG
		putchar(' ');
#endif DEBUG
		hexint(~csum, 1);
		putchar('\n');
	}
	if (address_len != 4 && data_base > (1 << (address_len * 8)))
		warning("address exceeds range of srec address field");
}

eofsrec(address_len, entrypt)
int address_len;
unsigned entrypt;
{
	unsigned csum;
	unsigned hexint();

	csum = 0;

	putchar('S');
	/* record type */
	putchar(hex[9 - (address_len - 2)]);
#ifdef DEBUG
	putchar(' ');
#endif DEBUG
	/* record length (address_len + csum_byte) */
	csum += hexint(address_len + 1, 1);
#ifdef DEBUG
	putchar(' ');
#endif DEBUG
	/* address */
	csum += hexint(entrypt, address_len);
#ifdef DEBUG
	putchar(' ');
	putchar(' ');
#endif DEBUG
	hexint(~csum, 1);
	putchar('\n');
}

unsigned
hexint(val, nbytes)
unsigned val;
int nbytes;
{
	unsigned csum = 0;
	unsigned byte;
	int i;

	for (i = nbytes - 1; i >= 0; i--) {
		byte = (val >> (i * 8)) & 0xff;
		csum += byte;
		putchar(hex[byte >> 4]);
		putchar(hex[byte & 0xf]);
	}
	return(csum);
}

void
error(msg, arg)
char *msg;
char *arg;
{
	fprintf(stderr, "%s: ERROR: ", progname);
	fprintf(stderr, msg, arg);
	putc('\n', stderr);
	exit(1);
}

void
warning(msg, arg)
char *msg;
char *arg;
{
	fprintf(stderr, "%s: WARNING: ", progname);
	fprintf(stderr, msg, arg);
	putc('\n', stderr);
}

unsigned digit();

/*
 * atob -- convert ascii to binary.  Accepts all C numeric formats.
 */
char *
atob(cp, iptr, base)
char *cp;
int *iptr;
unsigned base;
{
	int minus = 0;
	register int value = 0;
	unsigned d;

	*iptr = 0;
	if (!cp)
		return(0);

	while (isspace(*cp))
		cp++;

#ifdef notdef
	while (*cp == '-') {
		cp++;
		minus = !minus;
	}

	/*
	 * Determine base by looking at first 2 characters
	 */
	if (*cp == '0') {
		switch (*++cp) {
		case 'X':
		case 'x':
			base = 16;
			cp++;
			break;

		case 'B':	/* a frill: allow binary base */
		case 'b':
			base = 2;
			cp++;
			break;
		
		default:
			base = 8;
			break;
		}
	}
#else
	base = 16;
#endif

	while ((d = digit(*cp)) < base) {
		value *= base;
		value += d;
		cp++;
	}

	if (minus)
		value = -value;

	*iptr = value;
	return(cp);
}

/*
 * digit -- convert the ascii representation of a digit to its
 * binary representation
 */
unsigned
digit(c)
char c;
{
	unsigned d;

	if (isdigit(c))
		d = c - '0';
	else if (isalpha(c)) {
		if (isupper(c))
			c = tolower(c);
		d = c - 'a' + 10;
	} else
		d = 999999; /* larger than any base to break callers loop */

	return(d);
}
