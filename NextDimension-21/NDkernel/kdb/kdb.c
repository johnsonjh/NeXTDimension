
#include "i860/param.h"
#include "i860/pcb.h"
#include "i860/psl.h"
#include "i860/reg.h"
#include "i860/trap.h"
#include "vm/vm_param.h"


extern char *kdbsbrk();

/*
 *  kdb_init - initialize kernel debugger
 */

kdb_init()
{
}



/*
 *  kdb_kintr - received keyboard interrupt sequence
 *
 *  Queue software interrupt to transfer into the debugger at the
 *  first oppurtunity.  This is not done if we are already in the
 *  debugger or if the system was booted without the KDB boot flag.
 */

int kdbactive = 0;

kdb_kintr()
{
}



/*
 *  kdb_trap - field a TRACE or BPT trap
 */

int kdbtrapok = 0;
int kdbreturn = 1;

kdb_trap(apsl)
register int *apsl;
{
    extern struct pcb kdbpcb;
    register struct pcb *pcb = &kdbpcb;
    register int *locr0 = apsl;
    register int type = locr0[TRAPNO];
    extern struct proc *P;

    switch (type)
    {
	case T_TRCTRAP:
	case T_BPTFLT:
	case T_KDB_ENTRY:
	    break;

	default:
	{
	    extern char *trap_type[];
	    extern int TRAP_TYPES;
	    extern char *kdberrflg;

	    printf("kernel: ");
	    if ((unsigned)type >= TRAP_TYPES)
		printf("type %d", type);
	    else
		printf("%s", trap_type[type]);
	}
    }

    kdbactive++;
    cnpollc(TRUE);
    kdb( type, apsl - 22, NULL );
/*     kdb(type == T_TRCTRAP, apsl - 22); */
    cnpollc(FALSE);
    kdbactive--;
    return(kdbreturn);
}



/*
 *  kdbread - read character from input queue
 */

kdbread(x, cp, len)
register char *cp;
{
    *cp = cngetc();
    return(*cp != 04);
}



/*
 *  kdbwrite - send characters to terminal
 */

/* ARGSUSED */
kdbwrite(x, cp, len)
register char *cp;
register int len;
{
   int n;

   n = len;
   while (len--)
	cnputc(*cp++);
   return(n);
}



/*
 *   kdbrlong - read long word from kernel address space
 */

kdbrlong(addr, p)
long *addr;
long *p;
{
	*p = 0;
	kdbtrapok = 1;
	if ((long)addr & 3) {
		unsigned long val;
		unsigned char *ap;

		ap = (unsigned char *)addr;
		val = (ap[0] << 24) | (ap[1] << 16) | (ap[2] << 8) | ap[3];
		*p = val;
	} else
		*p = *addr;
	kdbtrapok = 0;
}



/*
 *  kdbwlong - write long word to kernel address space
 */

kdbwlong(addr, p)
register long *addr;
long *p;
{
	kdbtrapok = 1;
	if ((long)addr & 3) {
		long w0, w1, val;
		long *ap;
		int offs;

		offs = (long)addr & 3;
		ap = (long *)((long)addr - offs);
		w0 = ap[0];
		w1 = ap[1];
		val = *p;
		switch (offs) {
		case 1:
			w0 = (w0 & 0xff) | ((val << 8) & ~0xff);
			w1 = (w1 & ~0xff) | ((val >> 24) & 0xff);
			break;
		case 2:
			w0 = (w0 & 0xffff) | ((val << 16) & ~0xffff);
			w1 = (w1 & ~0xffff) | ((val >> 16) & 0xffff);
			break;
		case 3:
			w0 = (w0 & 0xffffff) | ((val << 24) & ~0xffffff);
			w1 = (w1 & ~0xffffff) | ((val >> 8) & 0xffffff);
			break;
		}
		kdb_wallow(ap,w0);
		kdb_wallow(ap+1,w1);
	} else {
		kdb_wallow(addr,*p);
	}
	kdbtrapok = 0;
}

kdb_wallow(addr,val)
long *addr;
long val;
{
	unsigned *p, db;
	extern vm_offset_t loadpt;
	extern char _etext;
	extern pt_entry_t *pmap_pte();
	pt_entry_t *ptp;
	pt_entry_t template;
	/*
	 * Treat writes to text specially, to get cache and TLB right.
	 */
	if (addr >= (long *)loadpt && addr <= (long *)&_etext){
		if ((ptp = pmap_pte(current_pmap(), addr)) == (pt_entry_t *)0)
			return;
		template = *ptp;
		if (template.modify == 0) {
			template.modify = 1;
			/* Remove write protection if needed */
			if ( (template.prot & i860_KRW) == 0 )
			{
				template.prot |= i860_KRW;
				*ptp = template;
				template.prot &= ~i860_KRW;
			}
			else 
				*ptp = template;
			invalidate_TLB();
		}
		*addr = val;
		flush_inv();
		/* Restore write protection if needed. */
		if ( (template.prot & i860_KRW) == 0 )
		{
			*ptp = template;
			invalidate_TLB();
		}
	} else
		*addr = val;
}


/*
 *  kdbsbrk - extended debugger dynamic memory
 */

static char kdbbuf[1024];
char *kdbend = kdbbuf; 

char *
kdbsbrk(n)
unsigned n;
{
    char *old = kdbend;

    if ((kdbend+n) >= &kdbbuf[sizeof(kdbbuf)])
    {
	return((char *)-1);
    }
    kdbend += n;
    return(old);
}

