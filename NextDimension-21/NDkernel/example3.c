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

 void
main()
{
    kern_return_t r;
    vm_address_t addr;
    vm_size_t size, offset;
    vm_address_t vpage;
    vm_offset_t ppage;
    int color;
    register double d;
    register double *daddr;
    
    union { int i[2]; double d; } hack;
    
    addr = 0;
#if 1
    size = 0x400000;
    r = vm_map_hardware( task_self(), &addr, 0x2E000000, size, TRUE );
    if ( r != KERN_SUCCESS )
    {
	    printf( "vm_map_hardware %D bytes: %D\n",size,  r );
	    exit( 1 );
    }
    vm_cacheable( task_self(), addr, size, TRUE );

    bzero( addr, size );
    size /= sizeof( double );
    while( 1 )
    {
    	color = ((offset << 8) | (offset << 20));
	hack.i[0] = color;
	hack.i[1] = color;
	d = hack.d;
	daddr = (double *)addr;
	LOOP32( size, *daddr++ = d );
    	color = ~color;
	hack.i[0] = color;
	hack.i[1] = color;
	d = hack.d;
	daddr = (double *)addr;
	LOOP32( size, *daddr++ = d );
	++offset;
    }
#if 0
    for ( vpage = addr; vpage < (addr + size); vpage += PAGE_SIZE )
    {
//	pmap_dump_range( vpage, vpage + PAGE_SIZE);
//	printf( "At addr 0x%X, read 0x%X\n", vpage, *((int *)vpage) );
	for ( offset = 0; offset < PAGE_SIZE; offset += sizeof (int) )
	    *((int *)(vpage + offset)) = ((offset << 8) | (offset << 20));
    }
#endif
#else
    size = 0x400000;
    if ( (r = vm_allocate( task_self(), &addr, size, TRUE )) != KERN_SUCCESS )
    {
	    printf( "vm_allocate %D bytes: %D\n",size,  r );
	    exit( 1 );
    }
    printf( "vm_allocate %D bytes gave us %D\n", size, addr );
#if 0
    r = vm_fault( current_pmap(), addr, VM_PROT_DEFAULT, FALSE );
    printf( "vm_fault returns %D\n" , r );
#endif
    pmap_dump_range( addr, addr + PAGE_SIZE);
    printf( "At addr 0x%X, read 0x%X\n", addr, *((int *)addr) );
    pmap_dump_range( addr, addr + PAGE_SIZE);
    printf( "Poking data...\n" );
    *((int *)addr) = 1;
    pmap_dump_range( addr, addr + PAGE_SIZE);
#endif
    exit( 0 );
}

