#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <mon/nvram.h>
#include <nextdev/video.h>

 static void
usage( char * prog )
{
        fprintf( stderr, "usage: %s [slot]\n", prog );
        exit( 1 );
}

main( int argc, char **argv )
{
    struct nvram_info nvram;
    char * program = argv[0];
    int fd;
    int slot;
        
    if( (fd = open( "/dev/vid0", O_RDWR )) < 0 )
    {
	    perror( "/dev/vid0" );
	    exit( 1 );
    }
    if( ioctl( fd, DKIOCGNVRAM, &nvram ) < 0 )
    {
	    perror( "DKIOCGNVRAM" );
	    close( fd );
	    exit( 1 );
    }
    if ( argc == 1 )
    {
	if ( nvram.ni_use_console_slot )
	    printf( "Console slot: %d\n", nvram.ni_console_slot << 1 );
	else
	    printf( "No console slot selected.\n" );
    }
    else
    {
    	if ( strcmp( argv[1], "-u" ) == 0 )
	{
	    nvram.ni_console_slot = 0;
	    nvram.ni_use_console_slot = 0;
	}
	else
	{
	    if ( ! isdigit( *argv[1] ) )
		    usage( program );
	    slot = atoi( argv[1] );
	    switch( slot )
	    {
		    case 0:
		    case 2:
		    case 4:
		    case 6:
			    break;
		    default:
			    usage( program );
	    }
	    nvram.ni_console_slot = slot >> 1;
	    nvram.ni_use_console_slot = 1;
        }
	if( ioctl( fd, DKIOCSNVRAM, &nvram ) < 0 )
	{
		perror( "DKIOCGNVRAM" );
		close( fd );
		exit( 1 );
	}
    }
    close( fd );
    exit( 0 );
}