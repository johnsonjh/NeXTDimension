#include <stdio.h>
#include <libc.h>
#include <math.h>

#define DEFAULT_GAMMA 2.0

main(argc, argv)
    int argc;
    char **argv;
{
    int index;
    double Gamma = DEFAULT_GAMMA;
    
    if ( argc > 1 )
    	Gamma = atof( argv[1] );
    
    printf( "unsigned char ND_GammaTable[] =\n{\n\t" );
    for ( index = 0; index < 256; ++index )
    {
	printf( "%d,",
	    (unsigned int)floor((pow(((double)index/255.0),(1.0/Gamma))*255.0)+ 0.5) );
	if ( (index & 7) == 7 )
		printf( "\n\t" );
	else
		printf( "\t" ); 
    }
    printf( "};\n" );
}



