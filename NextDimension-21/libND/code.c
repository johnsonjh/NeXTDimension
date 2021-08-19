#include "ND/ND_thread_status.h"	/* MUST BE FIRST! */
#include "ND/ND_machine.h"		/* MUST BE SECOND! */
#include <stdio.h>
#include <sys/errno.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include "ND/NDlib.h"
#include <sys/file.h>
#include <signal.h>
#include <setjmp.h>
#include <libc.h>
#include <mach.h>
#include <ldsyms.h>

static void thud( int );
static int __NDPLoadMach0__( ND_var_t * );
static jmp_buf	Whoops;

static void * LoadImage = (void *) 0;

struct thread_aout
{
	struct thread_command ut;
	unsigned long flavor;
	unsigned long count;
	struct NextDimension_thread_state_regs cpu;
};

/*
 *	The following macros are used to load the text and data 
 *	segments from the mapped in binary.  Things are complicated
 *	by the use of big-endian data and little-endian text.
 *	The 32 bit wide NextBus gives us access to all 32 bit words
 *	of the i860 memory.  Assuming that Adam and Ross went fully
 *	big-endian on the i860 and NextBus sides, the 
 *	packing of 32 bit words into 64 bit longwords is big-endian,
 *	and we are all set in the data segment.  We have to toggle the 
 *	word addresses for the text segment, as the text is handled little-endian
 *	by the i860.
 */
#if defined(TEST)
#define LOADDATA( addr, data )	printf("data %08x: %08x\n", addr, data )
#define LOADTEXT( addr, data )	printf("text %08x: %08x\n",(((unsigned)addr)^4),data);
#else
#define LOADDATA( addr, data )	(*((int *)(addr)) = (data))
#define LOADTEXT( addr, data )	(*((int *)(((unsigned)addr)^4)) = (data))
#endif

//
// The code to be booted should be linked to start execution at the base of DRAM
//
#define ND_vtoo(vaddr)	(((vaddr)&0x0FFFFFFF) - (P_ADDR_MEMORY & 0x0FFFFFFF))

 int
NDLoadCode( ND_var_p handle, char *file )
{
    int fd;
    struct stat stbuf;
    int retval;
        
    // Prep text and data for loading
    // First, map it in so we can hand a pointer to the Mach section/segment functions.
    
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
#if defined(TEST)
    printf( "%s mapped at 0x%08x for %d bytes\n", file, LoadImage, stbuf.st_size );
#endif
    retval = __NDPLoadMach0__(handle);
    
    vm_deallocate( task_self(), (vm_address_t) LoadImage, stbuf.st_size );
    close( fd );
    return( retval );
}

 int
NDLoadCodeFromSect( ND_var_p handle, struct mach_header *mhp, char *seg, char *sect )
{
    int size;
        
    // Prep text and data for loading
    // In this format, the entire i860 mach-0 is embedded within a section of the
    // host program.  This will let us do lots of sneaky things later...
    
    LoadImage = (void *)getsectdatafromheader( mhp, seg, sect, &size );
    if ( LoadImage == (void *) NULL )
    {
    	errno = ENOEXEC;
	return -1;
    }
#if defined(TEST)
    printf( "%s:%s mapped at 0x%08x for %d bytes\n",
    		seg, sect, LoadImage, size );
#endif
    return( __NDPLoadMach0__(handle) );
}

 static int
__NDPLoadMach0__( ND_var_p handle )
{
    struct mach_header *mhp;
    struct load_command *lcmd;
    struct segment_command *scmd;
    struct thread_aout *thread = (struct thread_aout *) NULL;
    char *data;
    int *isrc;
    int *idest;
    int ncmds;
    int cnt;
    int txtstart;
    volatile int *Daddr;
    int csr;
    void (*old_SIGBUS)();
    void (*old_SIGSEGV)();
        
    Daddr = ADDR_DRAM(handle);
    // First, evaluate the Mach-0 headers
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
    if ( mhp->filetype != MH_EXECUTE && mhp->filetype != MH_PRELOAD )
    {
    	errno = ENOEXEC;
    	return -1;
    }
    
    // Set up fault handler prior to loading code and data
    old_SIGBUS = signal( SIGBUS, thud );
    old_SIGSEGV = signal( SIGSEGV, thud );
    
    if ( setjmp(Whoops) != 0 )
    {
	    (void) signal( SIGBUS, old_SIGBUS );
	    (void) signal( SIGSEGV, old_SIGSEGV );
	    errno = EFAULT;
	    return -1;
    }
    //
    // In the non-PROTOTYPE version, code in boot.c issues a RESET to 
    // transfer the i860 back to running out of ROM in CS8 mode.  
    // When this code is run, the processor should already be reset.
    //
#if !defined(TEST)
#if defined(PROTOTYPE)
    csr = *ADDR_NDP_CSR(handle);	// Halt processor while we load code
    *ADDR_NDP_CSR(handle) = (csr | NDCSR_RESET860);
#endif /* !PROTOTYPE */
#endif   
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
	isrc = (int *)(((char *)LoadImage) + scmd->fileoff);
	idest = (int *)(((char *)Daddr) + ND_vtoo(scmd->vmaddr));
#if defined(TEST)
    printf( "Seg %s\n", scmd->segname );
#endif
	if ( strcmp( scmd->segname, "__TEXT" ) == 0 )
	{
	    // In MH_EXECUTE files, little-endian code starts after big-endian headers
	    if ( mhp->filetype == MH_EXECUTE && scmd->fileoff == 0 )
		txtstart = sizeof (struct mach_header) + mhp->sizeofcmds;
	    else
	    	txtstart = 0;
	    // Load Mach-0 headers as data
	    for ( cnt = 0; cnt < txtstart; cnt += sizeof (int) )
	    {
		LOADDATA(idest, *isrc);
		++idest;
		++isrc;
	    }
	    if ( scmd->flags & SG_HIGHVM )
	    {
		// Zero fill the portion of core not covered by the file.
		for(cnt=txtstart;cnt<(scmd->vmsize - scmd->filesize);cnt+=sizeof(int))
		{
		    LOADTEXT(idest, 0);
		    ++idest;
		}
		// Copy the portion of this segment in file to core
		for ( cnt = 0; cnt < scmd->filesize; cnt += sizeof (int) )
		{
		    LOADTEXT(idest, *isrc);
		    ++idest;
		    ++isrc;
		}
	    }
	    else
	    {
		// Copy the portion of this segment in file to core
		for ( cnt = txtstart; cnt < scmd->filesize; cnt += sizeof (int) )
		{
		    LOADTEXT(idest, *isrc);
		    ++idest;
		    ++isrc;
		}
		// Zero fill the portion of core not covered by the file.
		for(cnt = 0;cnt<(scmd->vmsize - scmd->filesize);cnt += sizeof (int))
		{
		    LOADTEXT(idest, 0);
		    ++idest;
		}
	    }
	}
	else if ( strcmp( scmd->segname, "__PAGEZERO" ) != 0 ) // Ignore __PAGEZERO
	{	// Data, Objective C, strings and such, all big-endian data
	    if ( scmd->flags & SG_HIGHVM )
	    {
		// Zero fill the portion of core not covered by the file.
		for(cnt = 0;cnt<(scmd->vmsize - scmd->filesize);cnt += sizeof (int))
		{
		    LOADDATA(idest, 0);
		    ++idest;
		}
		// Copy the portion of this segment in file to core
		for ( cnt = 0; cnt < scmd->filesize; cnt += sizeof (int) )
		{
		    LOADDATA(idest, *isrc);
		    ++idest;
		    ++isrc;
		}
	    }
	    else
	    {
		// Copy the portion of this segment in file to core
		for ( cnt = 0; cnt < scmd->filesize; cnt += sizeof (int) )
		{
		    LOADDATA(idest, *isrc);
		    ++idest;
		    ++isrc;
		}
		// Zero fill the portion of core not covered by the file.
		for(cnt = 0;cnt<(scmd->vmsize - scmd->filesize);cnt += sizeof (int))
		{
		    LOADDATA(idest, 0);
		    ++idest;
		}
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
    //
    // On the completed board, we don't load a vector or clear RESET.
    // The vector is in ROM, and RESET clears itself!   Instead, the code over in 
    // boot.c will set a semaphore read by the i860, which will transfer control to
    // the base of DRAM.
    //
#if defined(TEST)
    printf( "Entry point 0x%08x\n", thread->cpu.pc );
#endif
#if defined(PROTOTYPE)
    // Load up the reset vector with a branch to the code entry point.
    NDSetTrapVector( handle, (unsigned long) thread->cpu.pc );
#if !defined(TEST)
    *ADDR_NDP_CSR(handle) = csr;	// Restore the original CSR
#endif   
#endif /* PROTOTYPE */
 
    
    // All done.  Clean up and release resources
    (void) signal( SIGBUS, old_SIGBUS );
    (void) signal( SIGSEGV, old_SIGSEGV );
    return 0;
}
#ifdef BYTE_SWAP
#define OP_NOP		0x000000A0	// little-endian NOP
#else
#define OP_NOP		0xA0000000	// big-endian NOP
#endif
#define OP_BR_RAW	0x68000000	// Unswapped branch opcode
#define TRAP_VECTOR	0xFFFFFF00	// Address receiving control on a trap
#define OPMASK		0xFC000000	// Mask for branch opcode bits
	
 int
NDSetTrapVector( ND_var_p handle, unsigned long vector )
{
    unsigned long br_op;
    unsigned long tmp;
    unsigned char *buf;
    void (*old_SIGBUS)();
    void (*old_SIGSEGV)();
    volatile int * Addr;
    
    // Aim the address pointer at the reset vector in the i860 DRAM 
    Addr = ADDR_DRAM(handle);
    Addr = (int *)(((char *)Addr) + ND_vtoo(TRAP_VECTOR));
    
    // Compute branch relative address, in a form suitable for a br opcode.
    tmp = vector - ((unsigned long) TRAP_VECTOR);	// Get relative span
    tmp >>= 2;						// Convert to word addr
    tmp += -3;	// Adjust for 2 NOPs, + 1 for pc increment
    		// NOPs are here for a chip bug.
    tmp &= ~OPMASK;					// Mask out opcode bits
    tmp |= OP_BR_RAW;					// OR in the branch opcode
#ifdef BYTE_SWAP
    buf = (unsigned char *)&tmp;			// Byte swap to little-endian
    br_op = (buf[3] << 24) | (buf[2] << 16) | (buf[1] << 8) | (buf[0]);
#else
    br_op = tmp;
#endif

    // Set up fault handler prior to loading reset vector
    old_SIGBUS = signal( SIGBUS, thud );
    old_SIGSEGV = signal( SIGSEGV, thud );
    
    if ( setjmp(Whoops) != 0 )
    {
	    (void) signal( SIGBUS, old_SIGBUS );
	    (void) signal( SIGSEGV, old_SIGSEGV );
	    errno = EFAULT;
	    return -1;
    }
    
    //	Load the interrupt vector and it's surround
    LOADTEXT( Addr, OP_NOP );		// nop		; CHIP BUG
    ++Addr;
    LOADTEXT( Addr, OP_NOP );		// nop		; CHIP BUG
    ++Addr;
    LOADTEXT( Addr, br_op );		// br vector	; Branch to trap handler
    ++Addr;
    LOADTEXT( Addr, OP_NOP );		// nop		; Delay slot
    ++Addr;
    LOADTEXT( Addr, OP_NOP );		// nop		; Delay slot paranoia
    ++Addr;
    LOADTEXT( Addr, OP_NOP );		// nop		; Delay slot paranoia
    ++Addr;
    LOADTEXT( Addr, OP_NOP );		// nop		; Delay slot paranoia
    ++Addr;
    LOADTEXT( Addr, OP_NOP );		// nop		; Delay slot paranoia
    
    // All done.  Clean up and release resources
    (void) signal( SIGBUS, old_SIGBUS );
    (void) signal( SIGSEGV, old_SIGSEGV );

    return 0;
}

 static void
thud(sig)
    int sig;
{
	longjmp(Whoops, sig);
}

#ifdef TEST
main( argc, argv )
    int argc;
    char ** argv;
{
	char *file;
	if ( argc < 2 )
		file = "a.out";
	else
		file = argv[1];
	if ( _NDPLoadCodeFromSect( (volatile int *) 0, "__I860", "__i860" ) == -1 )
	{
		fprintf( stderr, "%s: load failure\n", file );
		perror( file );
	}
}
#endif
