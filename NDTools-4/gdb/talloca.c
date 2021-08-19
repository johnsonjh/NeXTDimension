char *alloca(size)
int size;
{
	char *sbrk();

	return(sbrk(size));
}
