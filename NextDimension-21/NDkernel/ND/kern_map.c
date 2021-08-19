#include <sys/map.h>
/*
 * Resource map management software.  A map consists of an array of structures,
 * with each structure containing a starting address and size for the resource.
 * A size entry of 0 identifies the end of a map.  Resource entries in the map
 * are sorted in increasing order.
 */
extern struct map *coremap;
extern unsigned long FreePageCnt;


 void
rminit( struct map *mp, unsigned nentries, char *name )
{
	struct mapent *bp = (struct mapent *) (mp + 1);
	
	bzero( (char *) mp, nentries * sizeof (struct mapent) );
	mp->m_limit = (struct mapent *)&mp[nentries];
	mp->m_name = name;
	bp->m_size = 0;		/* Terminate the map. */
	
	if ( mp == coremap )
		FreePageCnt = 0;
}

 void
rmdump( struct map *mp )
{
	struct mapent *bp = (struct mapent *) (mp + 1);
	printf( "Map %s, size %D\n", mp->m_name, mp->m_limit - bp );
	while ( bp->m_size && bp < mp->m_limit )
	{
		printf( "Frag: %D units at address 0x%X\n", bp->m_size, bp->m_addr );
		++bp;
	}
}

unsigned
rmalloc( struct map *mp, unsigned size)
{
	register unsigned a;
	register struct mapent *bp;

	for ( bp = (struct mapent *)(mp + 1); bp->m_size && bp < mp->m_limit; bp++ )
	{
		if ( bp->m_size >= size )	/* A fit has been found */
		{
			a = bp->m_addr;
			bp->m_addr += size;
			if ( (bp->m_size -= size) == 0 ) /* remove empty entry */
			{
				do	/* copy down remaining entries */
				{
					++bp;
					(bp - 1)->m_addr = bp->m_addr;
				}
				while ( ((bp - 1)->m_size = bp->m_size) != 0 );
			}
			if ( mp == coremap )
				FreePageCnt -= size;
			return a;
		}
	}
	if ( bp >= mp->m_limit )
	{
		printf( "Resource %s exhausted\n", mp->m_name );	
		(mp->m_limit - 1)->m_size = 0;	/* terminate list */
	}
	return 0;
}

/*
 * Free the resource at location aa of size 'size' into map mp.
 *
 * aa is sorted into the map, and combined on one or both ends if possible.
 */
 void
rmfree(struct map *mp, unsigned size, unsigned aa)
{
	register struct mapent *bp;
	register unsigned t;
	register unsigned a;
	unsigned freed = size;
	
	a = aa;
	bp = (struct mapent *)(mp + 1);
	while (  bp->m_addr <= a && bp->m_size != 0 && bp < mp->m_limit  )
		++bp;
		
	if ( bp >= mp->m_limit )
	{
		printf( "Resource %s exhausted (lost %D units at 0x%X)[1]\n",
			mp->m_name, size, aa );	
		(mp->m_limit - 1)->m_size = 0;	/* terminate list */
		return;
	}

	if ( bp > (struct mapent *)(mp+1) && ((bp-1)->m_addr + (bp-1)->m_size) == a )
	{	/* Grow frag up. */
		(bp - 1)->m_size += size;
		if ( (a + size) == bp->m_addr )		/* combine abutting frags */
		{
			(bp - 1)->m_size += bp->m_size;
			
			/* delete 0 length frag */
			while ( bp->m_size != 0 && bp < mp->m_limit)
			{
				++bp;
				(bp - 1)->m_addr = bp->m_addr;
				(bp - 1)->m_size = bp->m_size;
			}
			if ( bp >= mp->m_limit )
			{
			    printf( "Resource %s exhausted (lost %D units at 0x%X)[2]\n",
				    mp->m_name, size, aa );	
			    (mp->m_limit - 1)->m_size = 0;	/* terminate list */
			    return;
			}
		}
	}
	else
	{
		if ( (a + size) == bp->m_addr && bp->m_size )	/* Grow frag down */
		{
			bp->m_addr -= size;
			bp->m_size += size;
		}
		else if ( size != 0 )	/* Head insert the new fragment */
		{
			do
			{
				t = bp->m_addr;
				bp->m_addr = a;
				a = t;
				t = bp->m_size;
				bp->m_size = size;
				++bp;
			}
			while ( (size = t) != 0 && bp < mp->m_limit );
			if ( bp >= mp->m_limit )
			{
			    printf("Resource %s exhausted (lost %D units at 0x%X)[3]\n",
				    mp->m_name, size, aa );	
			    (mp->m_limit - 1)->m_size = 0;	/* terminate list */
			    return;
			}
		}
	}
	if ( mp == coremap )
		FreePageCnt += freed;
	
	Wakeup( (int) mp );	/* Wake anyone waiting for this resource. */
}

