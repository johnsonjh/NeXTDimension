#include <mach.h>
#include <servers/netname.h>
#include <stdio.h>
#include "ND/NDlib.h"

#define MFG_ID	0x89
#define DEV_ID	0xb4

main( argc, argv )
    int argc;
    char **argv;
{
    extern struct mach_header _mh_execute_header;
    port_t ND_port;
    char * program = argv[0];
    char *prog860 = "rom.out";
    int slot = -1;
    int slotbits, i;
    kern_return_t r;
    int erase_flag = 0;
    int check_flag = 0;
    int unit;
    int mfg, dev;
    unsigned sum;
    unsigned record_sum;
    
    while ( --argc )
    {
    	if ( **(++argv) == '-' )
	{
		if ( strcmp( *argv, "-s" ) == 0 && argc > 1)
		{
		    --argc;
		    slot = atoi( *(++argv) );
		}
		else if ( strcmp( *argv, "-erase" ) == 0 )
		{
		    erase_flag = 1;
		    check_flag = 0;
		}
		else if ( strcmp( *argv, "-check" ) == 0 )
		{
		    check_flag = 1;
		}
		else
		    usage( program );
	}
	else
		usage( program );
    }

    if ( (r = ND_Load_MachDriver()) != KERN_SUCCESS )
    {
    	mach_error( "ND_Load_MachDriver", r );
	exit( 1 );
    }
    
    r = netname_look_up(name_server_port, "", "NextDimension", &ND_port);
    if (r != KERN_SUCCESS)
    {
    	mach_error( "netname_lookup", r );
	exit( 1 );
    }
    
    /* If no slot was specified, try all slots. */
    if ( slot == -1 )
    {
    	for ( unit = 2; unit <= 6; unit += 2 )
	{
	    if (ND_ROM_Identifier(ND_port, unit, &mfg, &dev ) != KERN_SUCCESS)
		continue;
	    if ( mfg != MFG_ID || dev != DEV_ID )
	    {
	   	printf(	"Slot %d: Unknown part!: "
			"Mfg Code %02X, Dev Code %02X\n",
			unit, mfg, dev);
	    	if ( ! check_flag )
			continue;
	    }

	    
	    if ( erase_flag )
	    {
	    	if ( (r = ND_ROM_Erase( ND_port, unit )) != KERN_SUCCESS )
		{
			printf( "Slot %d: ROM Erase failed (%d).\n", unit, r );
			continue;
		}
	    }
	    else
	    {
	    	r = ND_ROM_CheckSum(ND_port, unit, &sum, &record_sum);
	    	if( r != KERN_SUCCESS )
		{
		    printf("Slot %d: ROM Check Sum call failed (%d).\n",
		    		unit,r);
		    continue;
		}
		if ( record_sum != sum )
		    printf("Slot %d: Bad Check Sum %08X != recorded %08X\n",
		    	 unit, sum, record_sum);
		else
		    printf("Slot %d: Check Sum %08X\n", unit, sum);

	    }
	}
    }
    else
    {
    	unit = slot;
	if ((r=ND_ROM_Identifier(ND_port, unit, &mfg, &dev )) != KERN_SUCCESS)
	{
		printf("Slot %d: ROM hardware error (%d).\n",unit,r);
		exit( 1 );
	}
	if ( mfg != MFG_ID || dev != DEV_ID )
	{
	    printf( "Slot %d: Unknown part!: Mfg Code %02X, Dev Code %02X\n",
		    unit, mfg, dev);
	    if ( ! check_flag )
		exit(1);
	}
	
	if ( erase_flag )
	{
	    if ( (r = ND_ROM_Erase( ND_port, unit )) != KERN_SUCCESS )
	    {
		printf( "Slot %d: ROM Erase failed (%d).\n", unit, r );
		exit(1);
	    }
	}
	else
	{
	    r = ND_ROM_CheckSum(ND_port, unit, &sum, &record_sum);
	    if( r != KERN_SUCCESS )
	    {
		printf("Slot %d: ROM Check Sum call failed (%d).\n",unit,r);
		exit(1);
	    }
	    if ( record_sum != sum )
		printf("Slot %d: Bad Check Sum %08X != recorded %08X\n",
			unit, sum, record_sum);
	    else
		printf("Slot %d: Check Sum %08X OK\n", unit, sum);
	}
	
    }

}

usage( char *s )
{
	fprintf( stderr, "Usage: %s [-erase] [-check]\n", s );
}