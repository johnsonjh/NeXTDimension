#include <sys/map.h>
#include <machine/vm_param.h>

/*
 * Malloc and free, in a form suitable for use inside the kernel.
 */

typedef double ALIGN;	/* Most restrictive alignment type */
union header
{
	struct
	{
		 union header	*ptr;	/* Next free block */
		 unsigned	size;	/* size of this free block */
		 unsigned	magic;	/* Magic number to spot malloc trashers */
		 unsigned 	padding; /* Keep aligned.... */
	}	s;
	ALIGN	x;			/* Force alignment of blocks */
};

#define MALLOC_MAGIC	0x03311986	/* Magic number for malloc tags. */

typedef union header HEADER;
static HEADER	base;			/* Empty list to get started */
static HEADER	*allocp = (HEADER *) 0;	/* Last allocated block */
static int 	addcore;		/* interlock between morecore() and kfree() */

#define NALLOC	((i860_PGBYTES + sizeof HEADER - 1) / sizeof HEADER )

static HEADER *morecore();
static void freecore();

dumpkalloc()
{
	register HEADER *p, *q;
	
	if ( (q = allocp) == (HEADER *)0 )
	{
		printf( "malloc pool: Nothing allocated yet\n" );
		return;
	}
	for ( p = q->s.ptr; ; q = p, p = p->s.ptr )
	{
	    printf( "malloc pool: %D bytes at 0x%X\n", p->s.size * sizeof(HEADER), p );
	    if ( p == allocp )	/* Wrapped around free list */
		return;
	}
}
 void *
kcalloc( nitems, nbytes )
    unsigned nitems, nbytes;
{
	void *cp;
	int size = nitems * nbytes;
	extern void *kmalloc();
	
	if ( (cp = kmalloc( size )) == (void *) 0 )
		return ((void *) 0);
	bzero( cp, size );
	return cp;
}

 void *
kmalloc( nbytes )
    unsigned nbytes;
{
	register HEADER *p, *q;
	register int nunits;
	
	/* Get # of HEADER sized units to allocate */
	nunits = 1 + ((nbytes + sizeof (HEADER) - 1)/sizeof (HEADER));
	if ( (q = allocp) == (HEADER *) 0 )	/* No free list yet */
	{
		base.s.ptr = allocp = q = &base;	/* Build 0 lenght free list */
		base.s.size = 0;
	}
	
	for ( p = q->s.ptr; ; q = p, p = p->s.ptr )
	{
		if ( p->s.size >= nunits )		/* Found a fit */
		{
			if ( p->s.size == nunits )	/* An exact fit? */
				q->s.ptr = p->s.ptr;
			else				/* Allocate tail end */
			{
				p->s.size -= nunits;
				p += p->s.size;
				p->s.size = nunits;
			}
			allocp = q;
			p->s.magic = MALLOC_MAGIC;
			return( (void *)(p + 1) );
		}
		if ( p == allocp )	/* Wrapped around free list */
		{
			if ( (p = morecore(nunits)) == (HEADER *)0 )
			{
				return( (void *) 0 );
			}
		}
	}
}

 static HEADER *
morecore( nu )
    unsigned nu;
{
	register char *cp;
	register HEADER *up;
	register int rnu;
	register unsigned page;
	extern struct map *coremap;
	extern unsigned long MinFreePages;
	
	rnu = round_page(nu * sizeof (HEADER));
	while ( (page = rmalloc( coremap, i860_btop(rnu) )) == 0 )
	{
		/* Wake page out daemon and sleep waiting for free pages */
		Wakeup( &coremap->m_name );
		Sleep( coremap, CurrentPriority() );
	}
	up = (HEADER *) i860_ptob(page);
	up->s.size = rnu / sizeof (HEADER);
	up->s.magic = MALLOC_MAGIC;		/* So free() will believe it. */
	addcore = 1;
	kfree( (char *)(up + 1) );		/* Drop into free list */
	addcore = 0;
	return( allocp );			/* Set by free() */
}

kfree( ap )
    void *ap;
{
	register HEADER *p, *q;
	register unsigned freesize = 0;
	extern struct map *coremap;
	
	p = (HEADER *)ap - 1;	/* Point to header structure */
	
	if ( p->s.magic != MALLOC_MAGIC )
	{
	    if ( KernelDebugEnabled() )
	    {
		printf( "Freeing unallocated/trashed storage (0x%X)\n", ap );
		abort();
	    }
	    return;
	}
	p->s.magic = 0;		/* Destroy magig number on free() */	
	for ( q = allocp; !(p > q && p < q->s.ptr); q = q->s.ptr )
	{
		if ( q >= q->s.ptr && (p > q || p < q->s.ptr) )
			break;	/* At one end or the other */
	}
	if ( p + p->s.size == q->s.ptr )	/* Merge with upper block */
	{
		freesize = (p->s.size += q->s.ptr->s.size);
		p->s.ptr = q->s.ptr->s.ptr;
	}
	else
		p->s.ptr = q->s.ptr;
		
	if ( q + q->s.size == p )		/* Merge with lower block */
	{
		freesize = (q->s.size += p->s.size);
		q->s.ptr = p->s.ptr;
	}
	else
		q->s.ptr = p;
	
	allocp = q;
	freesize *= sizeof (HEADER);
	if ( ! addcore && freesize && freesize == trunc_page(freesize) )
		freecore();
}

/*
 * Run through the kmalloc pool and release any full pages back
 * to the core resource map.
 */
 static void
freecore()
{
    register HEADER *p, *q;
    extern struct map *coremap;
    unsigned size;
	
    if( (q = allocp) == (HEADER *)0 )
    	return;
    for ( p = q->s.ptr; ; q = p, p = p->s.ptr )
    {
	/* Scan for a block of free pages. */
	if ( p->s.size )
	{
	    size = p->s.size * sizeof (HEADER);
	    if ( p == (HEADER *)trunc_page(p) && size == trunc_page(size) )
	    {
		q->s.ptr = p->s.ptr;
		allocp = q;
		rmfree( coremap, i860_btop(size), i860_btop(p) );
		return;
	    }
	}
	if ( p == allocp )		/* Wrapped around free list */
	    return;
    }
    return;
}

