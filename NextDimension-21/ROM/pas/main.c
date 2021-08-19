#include <stdio.h>
#include <ctype.h>
#include "pas.h"

extern char *CurrentFile;
extern int yylineno;
int ErrorCount = 0;
static char *outname();
struct tn *Forest = (struct tn *) NULL;
static char **FileList = (char **) NULL;

main( argc, argv )
    int argc;
    char **argv;
{
	int flags = 0;
	int nfiles = 0;
	int cnt;
	char *outfile = (char *) NULL;
	extern void *malloc();
	
	cnt = 1;
	while( cnt < argc )
	{
		if ( *argv[cnt] == '-' )
		{
			if ( strcmp( argv[cnt], "-list" ) == 0 )
				flags |= OUT_LIST;
			else if ( strcmp( argv[cnt], "-o" ) == 0 
					&& (cnt + 1) < argc )
			{
				outfile = argv[++cnt];
			}
			/* Add else if()s here as needed. */
			else
				usage( argv[0] );
		}
		else
		{
			++nfiles;
			if ( outfile == (char *) NULL )
				outfile = outname( argv[cnt] );
		}
		++cnt;
	}
	if ( nfiles == 0 )
		process_files( (char *) NULL, flags, outfile );
	else
	{
		FileList = (char **) malloc( (nfiles + 1) * sizeof (char *) );
		if ( FileList == (char **) NULL )
		{
			fprintf( stderr, "%s: Out of Memory!\n", argv[0] );
			exit( 1 );
		}
		nfiles = 0;
		for( cnt = 1; cnt < argc; ++cnt )
		{
			if ( *argv[cnt] != '-' && argv[cnt] != outfile )
				FileList[nfiles++] = argv[cnt];
		}
		FileList[nfiles] = (char *) NULL; /* NULL terminate the list */
		process_files( FileList[0], flags, outfile );
		free( FileList );
	}
	exit( ErrorCount );
}

process_files( name, flags, outfile )
    char *name;
    int flags;
    char *outfile;
{
	extern char *savestr();
	
	if ( name == (char *) NULL )
		CurrentFile = savestr( "(stdin)" );
	else
	{	/* Hook 1st file to stdin.  yywrap will get later files */
		CurrentFile = name;
		if ( freopen( name, "r", stdin ) == (FILE *) NULL )
		{
			++ErrorCount;
			perror( name );
			return;
		}
	}
	yyparse();
	
	if ( ErrorCount )
	{
		deforest();
		return;
	}
	/* At this point, we have the whole parsed source in memory. */

	if ( name == (char *) NULL )
		CurrentFile = "(stdin)";
	else
		CurrentFile = name;

	pass1();	/* Assign PCs, load up "symbol table" */
	pass2();	/* Get forward references */
	output( outfile, flags );
	deforest();
}

/*
 * Replacement yywrap() arranges for more input when we hit EOF.
 * Return 0 if more input is available, 1 if all done.
 */
yywrap()
{
	static int Count = 0;
	extern char *savestr();
	
	if ( FileList == (char **) NULL || FileList[++Count] == (char *) NULL )
		return( 1 );
	if ( freopen( FileList[Count], "r", stdin ) == (FILE *) NULL )
	{
		++ErrorCount;
		perror( FileList[Count] );
		return( 1 );
	}
	CurrentFile = FileList[Count];
	yylineno = 1;	/* Reset for new file. */
	return( 0 );
}

yyerror( s )
    char *s;
{
	extern int yychar;
	extern char yytext[];
	
	++ErrorCount;
	fprintf( stderr, "File %s, Line %d: %s", CurrentFile, yylineno, s );
	if ( isprint(yytext[0]) )
		fprintf( stderr, " on or before %s.\n", yytext );
	else if ( yytext[0] == '\n' )
		fprintf( stderr, " on this or previous line.\n" );
	else
		putc( '\n', stderr );
}

misc_warning( s, file, line )
    char *s;
    char *file;
    int line;
{
	fprintf( stderr, "Warning: File %s, Line %d: %s.\n", file, line, s );
}


misc_error( s, file, line )
    char *s;
    char *file;
    int line;
{
	++ErrorCount;
	fprintf( stderr, "File %s, Line %d: %s.\n", file, line, s );
}

fatal( s )
    char *s;
{
	fprintf( stderr, "File %s, Line %d: Fatal Error: %s\n",
			CurrentFile, yylineno, s );
	exit( 1 );
}

usage( prog )
    char *prog;
{
	fprintf( stderr, "Usage: %s [-list] srcfile [srcfile...]\n", prog );
	exit(1);	
}

 static char *
outname( name )
    char *name;
{
	char *cp1;
	char *cp2;
	char *copy;
	extern void *malloc();
	
	/* Alloc space for the copy + 2 chars + trailing NUL */
	if ( (copy = (char *)malloc( strlen(name) + 3 )) == (char *) NULL )
		fatal( "Out of Memory" );
	
	cp2 = name;
	cp1 = copy;
	while( *cp1 = *cp2++ )	/* copy thru NUL */
		++cp1;
	while ( cp1 > copy )
	{
		if ( *cp1 == '.' )
		{
			*cp1 = '\0';
			break;
		}
		--cp1;
	}
	strcat( copy, ".o" );
	return( copy );
}
