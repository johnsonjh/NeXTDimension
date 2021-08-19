#include <mach.h>
#include <sys/types.h>
#include <sys/kern_return.h>
#include <sys/message.h>
#include <vm/vm_param.h>
#include "ND/NDmsg.h"
#include "i860/proc.h"
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
    
    printf( "pmap 0x%X\n", current_pmap() );
    
    addr = 0;
#if 0
    size = 0x400000;
    if ( (r = vm_allocate( task_self(), &addr, size, TRUE )) != KERN_SUCCESS )
    {
	    printf( "vm_allocate %D bytes: %D\n",size,  r );
	    exit( 1 );
    }
    printf( "vm_allocate %D bytes gave us %D\n", size, addr );
    /* Wiring down the frame buffer... */
    for (	vpage = addr, ppage = 0x2b400000;
    		vpage < (addr + size);
		vpage += PAGE_SIZE, ppage += PAGE_SIZE )
    {
    	pmap_enter( current_pmap(), vpage, ppage, VM_PROT_DEFAULT, TRUE );
    }
    bzero( addr, size );
    for ( vpage = addr; vpage < (addr + size); vpage += PAGE_SIZE )
    {
//	pmap_dump_range( vpage, vpage + PAGE_SIZE);
//	printf( "At addr 0x%X, read 0x%X\n", vpage, *((int *)vpage) );
	for ( offset = 0; offset < PAGE_SIZE; offset += sizeof (int) )
	    *((int *)(vpage + offset)) = ((offset << 8) | (offset << 20));
    }
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

