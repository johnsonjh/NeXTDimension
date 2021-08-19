/*
 *	swab_pte:
 *
 *	This function is called with a pointer to a first or second level
 *	page table holding 1024 entries.  If the level flag is 1, then 
 *	each non-zero entry in the table is a 2nd level table pointer.
 *	Recurse through that table, then swap the pointer and update the 
 *	1st level table.
 *
 */
swab_pte( table, level )
    unsigned long *table;
    int level;
{
	int i;
	unsigned long pte0, pte1;
	
	for ( i = 0; i < 1024; i += 2 )
	{
	    pte0 = table[i];
	    pte1 = table[i + 1];
	    
	    if ( level == 1 )
	    {
		if ( pte0 != 0L )
		    swab_pte( (unsigned long *)(pte0 & ~0xFFF), 2 );
		if ( pte1 != 0L )
		    swab_pte( (unsigned long *)(pte1 & ~0xFFF), 2 );
	    }
	    table[i] = pte1;
	    table[i + 1] = pte0;
	}
	flush();
}
