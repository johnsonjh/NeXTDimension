#include <mach.h>
#include "ND/ND.h"
#include <stdio.h>
#include <servers/netname.h>
#include <sys/message.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/mman.h>
#include "ND_var.h"

typedef struct {
	    msg_header_t Head;
	    msg_type_t RetCodeType;
	    kern_return_t RetCode;
	    int	space[1024];
} genericMsg;
		

main(argc, argv)
	int argc;
	char *argv[];
{
    kern_return_t	r;
    port_t		ND_Port, ownerPort, debugPort, negotiationPort;
    int offset;
    int value;
    vm_address_t vaddr;
    vm_size_t size;
    ND_conio_t msg;
    thread_t thread;
    thread_array_t thread_list;
    int cnt, i;
    int slot = 2;
    char buff[256];
    
    sync();
    r = netname_look_up( name_server_port, "", "NextDimension", &ND_Port );
    
    if (r != NETNAME_SUCCESS)
    {
	mach_error( "netname_look_up", r );
    	exit( 1 );
    }
    printf( "NextDimension Port %d\n", ND_Port );
    sync();

    if ( (r = port_allocate( task_self(), &debugPort )) != KERN_SUCCESS )
	mach_error( "port_allocate debug", r );

    if ( (r = port_allocate( task_self(), &negotiationPort)) != KERN_SUCCESS)
	mach_error( "port_allocate negotiationPort", r );

    if ( (r = port_allocate( task_self(), &ownerPort)) != KERN_SUCCESS)
	mach_error( "port_allocate ownerPort", r );

    /*
	* Get ownership of the board.
	*/
    sync();
    r = ND_SetOwner(ND_Port, slot, ownerPort, &negotiationPort);
    if ( r != KERN_SUCCESS )
	mach_error( "ND_SetOwner", r );
    else
    	printf( "ND_SetOwner OK\n" );
    /* send the port to the kernel server */
    sync();

    if ( (r = ND_SetDebug( ND_Port, slot, debugPort )) != 0 )
	mach_error( "ND_SetDebug", r );
    else
    	printf( "ND_SetDebug OK\n" );

    if ((r = ND_vm_mapper( ND_Port, ownerPort, task_self(),
    				(vm_address_t *)&vaddr,
				(vm_address_t)0x2b400000,
				(vm_size_t) 0x400000 )) != 0)
	mach_error( "ND_vm_mapper", r );
    else
    {
    	printf( "ND_vm_mapper OK (0x%08x)\n", vaddr );
	if ( (r = vm_deallocate( task_self(), vaddr, 0x400000 )) != KERN_SUCCESS )
	    mach_error( "vm_deallocate", r );
    }
    
    if ((r=ND_MapFramebuffer( ND_Port, ownerPort, task_self(), slot, &vaddr, &size ))!=0)
	mach_error( "ND_MapFramebuffer", r );
    else
    {
    	printf( "ND_MapFramebuffer OK (0x%08x, %d bytes)\n", vaddr, size );
	if ( (r = vm_deallocate( task_self(), vaddr, size )) != KERN_SUCCESS )
	    mach_error( "vm_deallocate", r );
    }
	
    
    if ((r=ND_MapCSR( ND_Port, ownerPort, task_self(), slot, &vaddr, &size ))!=0)
	mach_error( "ND_MapCSR", r );
    else
    {
    	printf( "ND_MapCSR OK (0x%08x, %d bytes)\n", vaddr, size );
	if ( (r = vm_deallocate( task_self(), vaddr, size )) != KERN_SUCCESS )
	    mach_error( "vm_deallocate", r );
    }
	
    
    if ((r=ND_MapDRAM( ND_Port, ownerPort, task_self(), slot, &vaddr, &size ))!=0)
	mach_error( "ND_MapDRAM", r );
    else
    {
    	printf( "ND_MapDRAM OK (0x%08x, %d bytes)\n", vaddr, size );
	if ( (r = vm_deallocate( task_self(), vaddr, size )) != KERN_SUCCESS )
	    mach_error( "vm_deallocate", r );
    }
	
    
    if ((r=ND_MapGlobals( ND_Port, ownerPort, task_self(), slot, &vaddr, &size ))!=0)
	mach_error( "ND_MapGlobals", r );
    else
    {
    	printf( "ND_MapGlobals OK (0x%08x, %d bytes)\n", vaddr, size );
	if ( (r = vm_deallocate( task_self(), vaddr, size )) != KERN_SUCCESS )
	    mach_error( "vm_deallocate", r );
    }
    
    if ((r=ND_MapHostToNDbuffer( ND_Port, ownerPort, task_self(), slot, &vaddr, &size ))!=0)
	mach_error( "ND_MapHostToNDbuffer", r );
    else
    {
    	printf( "ND_MapHostToNDbuffer OK (0x%08x, %d bytes)\n", vaddr, size );
	if ( (r = vm_deallocate( task_self(), vaddr, size )) != KERN_SUCCESS )
	    mach_error( "vm_deallocate", r );
    }

    if ( (r = ND_ResetMessageSystem( ND_Port, slot, ownerPort )) != 0 )
	mach_error( "ND_ResetMessageSystem", r );
    else
    	printf( "ND_ResetMessageSystem OK\n" );
	
    if ( (r = ND_GetKernelPort( ND_Port, slot, &debugPort )) != 0 )
	mach_error( "ND_GetKernelPort", r );
    else
    	printf( "ND_GetKernelPort OK (%d)\n", debugPort );
	
    if ( (r = ND_Port_look_up( ND_Port, slot, "kernel", &debugPort )) != 0 )
	mach_error( "ND_Port_look_up", r );
    else
    	printf( "ND_Port_look_up OK (%d)\n", debugPort );
	
    if ( (r = ND_GetBoardList( ND_Port, &i )) != 0 )
	mach_error( "ND_GetBoardList", r );
    else
    	printf( "ND_GetBoardList OK (0x%08x)\n", i );

    if ( (r = ND_PortTranslate( ND_Port, ND_Port, &value )) != 0 )
	mach_error( "ND_PortTranslate", r );

    printf( "User port %d == kernel port %d\n", ND_Port, value );

    if ( (r = ND_PortTranslate( ND_Port, debugPort, &value )) != 0 )
	mach_error( "ND_PortTranslate", r );

    printf( "User port %d == kernel port %d\n", debugPort, value );

    port_deallocate( task_self(), ND_Port );
    exit(0);
    
}
