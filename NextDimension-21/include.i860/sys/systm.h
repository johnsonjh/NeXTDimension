/*
 * Structure of the system-entry table
 */
extern struct sysent
{
	short	sy_narg;		/* total number of arguments */
	short	sy_parallel;		/* can execute in parallel */
	int	(*sy_call)();		/* handler */
} sysent[];


