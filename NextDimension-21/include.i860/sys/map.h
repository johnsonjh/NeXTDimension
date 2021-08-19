/*
 * Generic kernel resource map structure.
 */
struct map	/* Must have same size as struct mapent. */
{
	struct mapent *m_limit;
	char *m_name;		/* For out of resources warning */
};

struct mapent
{
	unsigned m_size;
	unsigned m_addr;
};

void rminit(struct map *, unsigned, char *);
unsigned rmalloc( struct map *, unsigned);
void rmfree(struct map *, unsigned, unsigned);

