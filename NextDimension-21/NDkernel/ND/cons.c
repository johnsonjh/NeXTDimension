#include <ND/NDreg.h>
#include <ND/ND_conio.h>

#if defined(PROTOTYPE)
volatile int * __ND_csr = (volatile int *)0x10;
#endif

int cons_simul = 0;
int cn_nulls = 0;

cnputc(c)
char c;
{

	if (cn_putc(c) == '\n')
		cn_putc('\r');
	return(c);
}

cn_putc(c)
char c;
{
	volatile char *buf = (volatile char *) abs_addr( ND_CONIO_OUT_BUF );
	volatile int *cnt = (volatile int *) abs_addr( ND_CONIO_OUT_COUNT );
	volatile int *id = (volatile int *) abs_addr( ND_CONIO_MSG_ID );
	/* Check for a full buffer and wait for it to drain. */
	while ( ND_IS_INTCPU )
		;
	
	buf[0] = c;
	buf[1] = 0;
	*cnt = 1;
	*id = ND_CONIO_OUT;
	
	ND_INTCPU;
	while( *id != ND_CONIO_IDLE )
		;
	return(c);
}

cngetc()
{
	register int c;

	c = cn_getc();
	if (cn_putc(c) == '\r')
		cn_putc(c = '\n');
	return(c);
}

cn_getc()
{
	int c;
	volatile char *outbuf = (volatile char *) abs_addr( ND_CONIO_OUT_BUF );
	volatile char *inbuf = (volatile char *) abs_addr( ND_CONIO_IN_BUF );
	volatile int *outcnt = (volatile int *) abs_addr( ND_CONIO_OUT_COUNT );
	volatile int *incnt = (volatile int *) abs_addr( ND_CONIO_IN_COUNT );
	volatile int *id = (volatile int *) abs_addr( ND_CONIO_MSG_ID );
	
	/* Check for a full buffer and wait for it to drain. */
	while ( ND_IS_INTCPU )
		;
	
	outbuf[0] = 0;
	*outcnt = 0;
	*id = ND_CONIO_IN_CHAR;
	*incnt = 0;
	
	ND_INTCPU;

	while( *id != ND_CONIO_IDLE )
		;

	c = *inbuf;
	return c;
}

cnpollc(){}

int rints = 0;
cnrintr()
{
	rints++;
}


puts(s)
char *s;
{
	s = (char *) abs_addr( s );
	while ( *s != '\0' )
		cnputc( *s++ );
	
}

panic( s )
    char *s;
{
	puts( "panic: " );
	puts( s );
	cnputc('\n');
	abort();
}

putx(n)
{
	int i, j;

	cn_putc('<');
	for (i=28; i>= 0; i-=4) {
		j = (n>>i) & 0xf;
		if (j <= 9)
			j += '0';
		else
			j += 'a'-10;
		cnputc(j);
	}
	cn_putc('>');
	cn_putc(' ');
}

_exit(status)
    int status;
{
	volatile char *buf = (volatile char *) abs_addr( ND_CONIO_OUT_BUF );
	volatile int *cnt = (volatile int *) abs_addr( ND_CONIO_OUT_COUNT );
	volatile int *id = (volatile int *) abs_addr( ND_CONIO_MSG_ID );
	/* Check for a full buffer and wait for it to drain. */
	while ( ND_IS_INTCPU )
		;
	
	buf[0] = status;
	buf[1] = 0;
	*cnt = 1;
	*id = ND_CONIO_EXIT;

#if defined(PROTOTYPE)
	*ADDR_NDP_CSR |= (NDCSR_INTCPU | NDCSR_RESET860);
#else /* !PROTOTYPE */
	ND_INTCPU;

	while ( ND_IS_INTCPU )
		;
	
#endif /* !PROTOTYPE */
	while ( 1 )
		;
}

/*
 * Send an interrupt to the host, telling the host that there is a message available.
 */
ping_driver()
{
	volatile int *id = (volatile int *) abs_addr( ND_CONIO_MSG_ID );
	/* Wait for last interrupt to clear */
	while ( ND_IS_INTCPU )
		;
	
	*id = ND_CONIO_MESSAGE_AVAIL;
	ND_INTCPU;
}

cache_on()
{
	/* Check for IO in progress. Wait for completion. */
	while ( ND_IS_INTCPU )
		;
	/* Enable the i860 cache */
	ND_KEN860;
}



