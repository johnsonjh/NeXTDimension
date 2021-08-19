#include <stdio.h>
#include <mach.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <a.out.h>
#include <sys/loader.h>
#include <ldsyms.h>

void	usage( char * );
void	mapin_file( char * );
static void process( char * );
extern const
struct section * getsectbynamefromheader( struct mach_header *, char *, char *);

char *	mapped_file;

struct nlist *sort_syms;
long nsort_syms;

struct nlist *syms;
char *str;

main( argc, argv )
    int argc;
    char **argv;
{
	char * target = "a.out";
	char * program = argv[0];
	
	if ( argc == 2 )
		target = argv[1];
	else if ( argc > 2 )
		usage( program );
	
	mapin_file( target );
	process( target );
	exit( 0 );
}

 void
mapin_file( char * file )
{
    int fd;
    struct stat stbuf;
    int retval;
        
    // Prep text and data for loading
    // First, map it in so we can hand a pointer to the Mach section/segment functions.
    
    if ( (fd = open( file, O_RDONLY, 0644 )) == -1 )
    {
    	perror( file );
	exit( 1 );
    }
    if ( fstat( fd, &stbuf ) == -1 )
    {
    	perror( file );
    	close( fd );
	exit( 1 );
    }
    
    if ( map_fd(fd, (vm_offset_t)0, (vm_offset_t *)&mapped_file,
    		1, (vm_size_t)stbuf.st_size ) != KERN_SUCCESS )
    {
    	perror( file );
    	close( fd );
	exit( 1 );
    }
}

 void
usage( char *program )
{
	fprintf( stderr, "Usage: %s [objfile]\n", program );
	exit( 1 );
}

/*
 * Function for qsort for comparing symbols.
 */
static
long
sym_compare(sym1, sym2)
struct nlist *sym1, *sym2;
{
    return(sym1->n_value - sym2->n_value);
}

/*
 * Function for qsort for comparing symbols.
 */
static
long
symname_compare(sym1, sym2)
struct nlist *sym1, *sym2;
{
    return(strcmp(sym1->n_un.n_name, sym2->n_un.n_name));
}

/*
 * Get the infomation about the symbol table of a mach object file
 */
static
void
get_sym_info(long *offset, long *size, long *num)
{
	long i;
	struct symtab_command *st;
	struct mach_header *mh = (struct mach_header *) mapped_file;
	struct load_command *lc = (struct load_command *)(mh + 1);

	*offset = 0;
	*size = 0;
	*num = 0;

	for(i = 0 ; i < mh->ncmds; i++){
	    if(lc->cmdsize % sizeof(long) != 0)
		printf("load command size not a multiple of sizeof(long)\n");
	    if(lc->cmd == LC_SYMTAB){
		st = (struct symtab_command *)lc;
		*offset = st->symoff;
		*size = st->nsyms * sizeof(struct nlist);
		*num = st->nsyms;
		return;
	    }
	    if(lc->cmdsize <= 0)
		break;
	    lc = (struct load_command *)((char *)lc + lc->cmdsize);
	}
}

/*
 * Get the infomation about the string table of a mach object file
 */
static
void
get_str_info(long *offset, long *size)
{
	long i;
	struct mach_header *mh = (struct mach_header *) mapped_file;
	struct load_command *lc = (struct load_command *)(mh + 1);
	struct symtab_command *st;

	*offset = 0;
	*size = 0;

	for(i = 0 ; i < mh->ncmds; i++){
	    if(lc->cmdsize % sizeof(long) != 0)
		printf("load command size not a multiple of sizeof(long)\n");
	    if(lc->cmd == LC_SYMTAB){
		st = (struct symtab_command *)lc;
		*offset = st->stroff;
		*size = st->strsize;
		return;
	    }
	    if(lc->cmdsize <= 0)
		break;
	    lc = (struct load_command *)((char *)lc + lc->cmdsize);
	}
}


static
void
process( char * filename )
{
    struct mach_header *mhp = (struct mach_header *) mapped_file;
    struct load_command *lcp;
    const struct section *scp;
    long sym_offset, sym_size, i, len, type;
    long nsyms;
    long str_offset, str_size;
    char *str, *p;
    char * output;
    int output_len;
    char *op;
    long *lp;
    off_t offset;
    int fd;
    struct nlist *symtable = (struct nlist *)0;

    if ( mhp->magic != MH_MAGIC )
    {
    	fprintf( stderr, "%s is not a Mach-0 file.\n" );
	exit ( 1 );
    }
    lcp = (struct load_command *)(mhp + 1);
    get_sym_info(&sym_offset, &sym_size, &nsyms);
    syms = (struct nlist *)(mapped_file + sym_offset);
    
    get_str_info(&str_offset, &str_size);
    str = mapped_file + str_offset;

    if((sort_syms = (struct nlist *)malloc(sym_size)) == (struct nlist *)0)
    {
	perror( filename );
	exit(1);
    }
    
    /*
     * Run through the namelist, binding strings to entries
     * and weeding out junk.
     */
    nsort_syms = 0;
    for(i = 0; i < nsyms; i++)
    {
	if(syms[i].n_un.n_strx > 0 && syms[i].n_un.n_strx < str_size)
	    p = str + syms[i].n_un.n_strx;
	else
	    continue;	/* No string? Pass on it. */
	if ( *p == '.' && *(p + 1) == 'L' )	/* No text locals, please */
	    continue;
	if ( strcmp( p, "gcc_compiled." ) == 0 )
	    continue;
	syms[i].n_un.n_name = p;
	/* Only keep C local syms. */
	if( (syms[i].n_type & ~(N_TYPE|N_EXT)) && *p != '_' )
	    continue;
	type = syms[i].n_type & N_TYPE;
	if(type == N_TEXT || type == N_DATA || type == N_BSS || type == N_SECT )
	{
	    len = strlen(p);
	    if((type == N_TEXT ||
		(type == N_SECT && syms[i].n_sect == 1) ) &&
	       len > sizeof(".o") - 1 &&
	       strcmp(p + (len - (sizeof(".o") - 1)), ".o") == 0){
		continue;
	    }
	    sort_syms[nsort_syms++] = syms[i];
	}
    }
    qsort(sort_syms, nsort_syms, sizeof(struct nlist), sym_compare);

    output_len = nsort_syms * sizeof (long);
    for ( i = 0; i < nsort_syms; ++i )
    {
/* printf( "%08x	%s\n", sort_syms[i].n_value, sort_syms[i].n_un.n_name ); */
    	output_len += (((strlen( sort_syms[i].n_un.n_name ) + 1) + sizeof (long) - 1)
			& ~(sizeof (long) - 1) );
	if ( strcmp( sort_syms[i].n_un.n_name, "_symtable" ) == 0 )
	{
		symtable = &sort_syms[i];
	}
    }
    if ( symtable == (struct nlist *) 0 )
    {
    	fprintf( stderr, "%s does not have a _symtable array to load!\n", filename );
	exit( 1 );
    }
    /*
     * Allocate an output buffer to hold the offset/name pairs.
     */
    if((output = (char *)malloc(output_len)) == (char *)0)
    {
	perror( filename );
	exit(1);
    }
    /*
     * Copy the core address (n_value) and name, long padded, to the buffer.
     */
    op = output;
    for ( i = 0; i < nsort_syms; ++i )
    {
    	lp = (long *)op;
	*lp++ = sort_syms[i].n_value;
	op = (char *)lp;
	len = strlen( sort_syms[i].n_un.n_name ) + 1;
	strcpy( op, sort_syms[i].n_un.n_name );
	op += len;
	while( len & (sizeof (long) - 1) )
	{
		*op++ = '\0';
		++len;
	}
    }
    /*
     * Find the location of _symtable in the target file.
     */
    if ( (scp = getsectbynamefromheader( mhp, SEG_DATA, SECT_DATA )) == NULL )
    {
    	fprintf( stderr, "Cannot find data section in %s,\n", filename );
	exit( 1 );
    }
    /*
     * The file offset is computed assuming that the offset of _symtable in the 
     * data section can be added to the file offset of the data section start
     * to get the file offset of _symtable.
     */
    offset = (symtable->n_value - scp->addr) + scp->offset;
    /*
     * Write the output buffer into the reserved storage at _symtable.
     */
    if ( (fd = open( filename, O_WRONLY, 0644 )) == -1 )
    {
    	perror( filename );
	exit( 1 );
    }
    if ( lseek( fd, offset, L_SET ) == -1 )
    {
    	perror( filename );
	exit( 1 );
    }
    if ( output_len > 50000 )	/* Too big to fit in object buffer? Truncate! */
    {
    	output_len = 50000;
	output[49999] = '\0';
	fprintf( stderr, "WARNING: %s symbol data too large.  Data truncated.\n",
		filename );
    }
    if ( write( fd, output, output_len ) != output_len )
    	fprintf( stderr, "WARNING: Write of symbol data to %s failed.\n",
		filename );
    close( fd );
    free( output );
}


