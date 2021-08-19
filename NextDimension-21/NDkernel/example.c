#include <mach.h>
#include <sys/types.h>
#include <sys/kern_return.h>
#include <sys/message.h>
#include <vm/vm_param.h>
#include "ND/NDmsg.h"
#include "i860/proc.h"
#include <nextdev/td.h>

struct Msg
{
	msg_header_t	Head;
	char 		Body[MSG_SIZE_MAX - sizeof (msg_header_t)];
} Msg;
int MsgCnt;

int fb_size;
unsigned int *fb_addr;

 void
main()
{
    kern_return_t r;
    vm_address_t addr;
    vm_size_t size, offset;
    int i;
    extern volatile int lbolt;
    int last;
    
    size = 0x2000000;
    if ( (r = vm_allocate( task_self(), &addr, size, TRUE )) != KERN_SUCCESS )
    {
	    printf( "vm_allocate %D bytes: %D\n",size,  r );
	    exit( 1 );
    }
    printf( "vm_allocate %D bytes gave us %D\n", size, addr );
    printf( "Reading new memory...\n" );
    for ( offset = 0; offset < size; offset += 0x1000 )
    {
	if ( *((int *)(addr + offset)) != 0 )
	{
	    printf( "Error: va 0x%X (pa 0x%X)read 0x%X, should be 0x%X\n",
		    addr + offset, kvtophys(addr + offset),
		    *((int *)(addr + offset)),addr+offset);
	}
    }
    printf( "Done\n" );
    for ( i = 0; i < 10; ++i )
    {
	printf( "Dirtying pages...\n" );
	last = lbolt;
	for ( offset = 0; offset < size; offset += 0x1000 )
	{
		*((volatile int *)(addr + offset)) = addr + offset;
	}
	last = lbolt - last;
	printf( "\t%D Kbytes in %D seconds, %D Kb/sec\n",
		size / 1024, last, (size/1024)/last );
	printf( "Read back test:\n" );
	last = lbolt;
	for ( offset = 0; offset < size; offset += 0x1000 )
	{
	    if ( *((volatile int *)(addr + offset)) != (addr + offset) )
	    {
		printf( "Error: va 0x%X (pa 0x%X)read 0x%X, should be 0x%X\n",
			addr + offset, kvtophys(addr + offset),
			*((volatile int *)(addr + offset)),addr+offset);
	    }
	}
	last = lbolt - last;
	printf( "\t%D Kbytes in %D seconds, %D Kb/sec\n",
		size / 1024, last, (size/1024)/last );
    }
    exit( 0 );
}

