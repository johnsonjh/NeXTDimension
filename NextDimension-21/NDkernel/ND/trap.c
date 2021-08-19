#include "i860/reg.h"
#include "i860/trap.h"
#include "i860/psl.h"
#include "i860/proc.h"
#include "ND/NDmsg.h"
#include "ND/NDreg.h"
#include <vm/vm_param.h>
#include <vm/vm_prot.h>
#include <sys/systm.h>
#include <sys/types.h>
#include <sys/exception.h>
#include <sys/errno.h>
#include <sys/kern_return.h>

/* macro to flip pointers between little-endian and big-endian spaces. */
#define LE_BE( addr, type )	((type)(((unsigned long)(addr))^4))

char	*trap_type[] = {
	"Reserved Addressing",
	"Priv Instr",
	"Reserved Operand",
	"Breakpoint",
	"XFC",
	"Syscall",
	"Floating Fault",
	"AST",
	"Segmentation",
	"Protection",
	"Trace trap",
	"Wild FSR",
	"Page fault",
	"Instr fault",
	"Read fault",
	"Write fault",
	"Enter kdb",
	"HW interrupt"
};

char	*det_name[] = {
	"Not Present",
	"Data break",
	"Alignment",
	"Dirty",
	"Permission",
	"Kernel text write",
	"Unknown",
	"User text write",
};
/*
 *  Need this accessible to the kernel debugger.
 */
int	TRAP_TYPES =	(sizeof trap_type / sizeof trap_type[0]);
int do_dirty = 1;
int at_debug = 0;

int *TrapStack;
int PageTraceFlags;

extern struct	sysent	sysent[];
extern int	nsysent;
extern char	*syscallnames[];

extern struct proc *P;	/* The current process */
extern int runrun;	/* The need to reschedule flag */

#ifdef SYSCALLTRACE
int syscalltrace = 0;
#endif

#if OLD_TRAP
trap(locr0,ins,va,iswrite,ttype,aln)
int *locr0;
{
	flush();
#if 0
	printf( "locr0 %X, ins %X, va %X\n", locr0, ins, va );
	printf( "iswrite %d, ttype %d, aln %d\n", iswrite, ttype, aln );
#endif
	switch (ttype) {
	case T_INTRPT:
		do_intr(locr0);
		break;
	default:
		kdb_trap(locr0);
		break;
	}
	fixes(locr0);
}
#endif
#if 1

/* For our purposes, code is user mode if below the kernel address space. */
#define USERMODE(pc)	((pc) < VM_MAX_ADDRESS && (pc) >= VM_MIN_TEXT_ADDRESS)

trap(locr0,ins,va,iswrite,ttype,aln)
int *locr0;
vm_offset_t va;
{
	extern char stext, sdata, bad;
#if 0
	extern vm_offset_t at_vaddr_start, at_vaddr_end;
	vm_map_t	map;
#endif
	extern int	ALLOW_FAULT_START,
			ALLOW_FAULT_END,
			FAULT_ERROR;
	int		save_error;
	int		*ptep, pdp;
	int		detail, psr, s;
	int		exc_type, exc_code, exc_subcode;
	vm_offset_t	recover;
	vm_offset_t	pc;
	int		result;
	struct proc	*p;

	TrapStack = locr0;	/* For debug purposes */

	if (((int)locr0 & 0xfff) < 0x300) {
		puts("Kernel stack overflow iminent: sp ");
		putx(locr0);
		cnputc('\n');
		kdb_trap(locr0);
	}
#ifdef	SYSCALLTRACE
	if (syscalltrace < -1 && ttype != T_INTRPT)
		printf("trap(%s, %X=>%X:%X)\n", trap_type[ttype], locr0[PC], va, locr0);
#endif	SYSCALLTRACE
	
	psr = locr0[PSR];
	switch (ttype) {

	case T_PAGEFLT:
	/* determine real reason for trap */
		if ((int)va & aln) {
			detail = DAT_ALN;
		} else if ((psr & (iswrite ? PSR_BW : PSR_BR)) &&
			(((int)va & ~aln) == (locr0[DB] & ~aln))) {
			detail = DAT_BR;
		} else {
			int pde, pte, perm;

			pdp = get_dirbase();
			pdp &= PTE_PFN;
			pde = *((int *)phystokv(pdp) + pdenum(va));
			if ((pde & PTE_PRESENT) == 0) {
				detail = DAT_INV;
				goto out;
			}
			perm = pde & (PTE_WRITABLE|PTE_USER);
			pde &= PTE_PFN;
			ptep = (int *)phystokv(pde) + ptenum(va);
			pte = *ptep;
			if ((pte & PTE_PRESENT) == 0) {
				detail = DAT_INV;
				goto out;
			}
#if 0
printf( "EPSR 0x%X\n", get_epsr() );
printf("PTE 0x%X (0x%X), ", pte, ptep );
pmap_dump_range( trunc_page((vm_offset_t)ptep), trunc_page(((vm_offset_t)ptep) + PAGE_SIZE) );
printf("PDE 0x%X (0x%X), ", *((int *)pdp + pdenum(va)), ((int *)pdp + pdenum(va)) );
pmap_dump_range( trunc_page((vm_offset_t)((int *)pdp + pdenum(va))),
		trunc_page(((vm_offset_t)((int *)pdp + pdenum(va))) + PAGE_SIZE) );
printf("DAT : ");
printf("psr %X ", psr);
printf("pc %X ", locr0[PC]);
printf(iswrite ? "write" : "read");
printf(" aln %D", aln);
printf(" ins %X", ins);
printf(" eff addr %X\n", va);
#endif
			perm &= pte & PTE_WRITABLE;
			if (iswrite && (pte & PTE_DIRTY) == 0) {
				if ((unsigned)va >= (unsigned)&stext &&
				    (unsigned)va <= (unsigned)&sdata) {
					detail = DAT_KTW;
				}
				else if (USERMODE(locr0[PC]) &&
					va >= VM_MIN_TEXT_ADDRESS &&
					va < (VM_MAX_ADDRESS - (1024*1024))) {
					detail = DAT_UTW;
				} else {
				    if (do_dirty) {
#ifdef	SYSCALLTRACE
					if (syscalltrace < -2)
						printf("D%x ", va);
#endif	SYSCALLTRACE
#if 0
					if (va >= at_vaddr_start &&
					    va < at_vaddr_end && at_debug)
						kdb(ttype, locr0, 0);
#endif
					if ( (PageTraceFlags & 2) != 0 )
						kdb(ttype, locr0, 0);
					pte |= PTE_DIRTY;
					*ptep = pte;	/* Update PTE */
					invalidate_TLB();
					fixes(locr0);
					return;
				    } else
					detail = DAT_DIRTY;
				}
				goto out;
			}
			if (iswrite && !(perm & PTE_WRITABLE)) {
				detail = DAT_PERM;
				goto out;
			}
			detail = DAT_UNKNOWN;
		}
out:
		if (detail == DAT_INV || detail == DAT_PERM)
			goto pageflt;
	default:			/* intentional fall through */
		if (USERMODE(locr0[PC]))
			break;
		printf("DAT : ");
		printf("psr %X ", psr);
		printf("pc %X ", locr0[PC]);
		printf(iswrite ? "write" : "read");
		printf(" aln %D", aln);
		printf(" ins %X", ins);
		printf(" eff addr %X", va);

		printf(" detail %s\n", det_name[detail]);
	case T_TRCTRAP:		/* intentional fall through */
	case T_BPTFLT:
	case T_KDB_ENTRY:
		kdb(ttype, locr0, 0);
		fixes(locr0);
		return;
	case T_INSFLT:
		va = locr0[PC];
	pageflt:
		if (P != (struct proc *) 0) {
			save_error = P->p_error;
			P->p_error = 0;
		}
#if 0
		printf("%s: ", trap_type[ttype]);
		printf("psr %X ", psr);
		printf("pc %X ", locr0[PC]);
		printf(iswrite ? "write" : "read");
		printf(" aln %D", aln);
		printf(" ins %X", ins);
		printf(" eff addr %X", va);

		printf(" detail %s\n", det_name[detail]);
		kdb(ttype, locr0, 0);
		fixes(locr0);
		return;
#else	/* Add a pager */
		/* Bring in the referenced page. */
		result = vm_fault(P->p_dirbase, trunc_page(va),
				iswrite ? VM_PROT_READ|VM_PROT_WRITE
					: VM_PROT_READ,
				FALSE);
		if (P != (struct proc *) 0) {
			P->p_error = save_error;
		}
		/* If the access was a write, go ahead and mark as dirty now. */
		if (result == KERN_SUCCESS) {
		    if (do_dirty && iswrite) {
#ifdef	SYSCALLTRACE
			if (syscalltrace < -2)
				printf("D%x ", va);
#endif	SYSCALLTRACE
			if ( (PageTraceFlags & 1) != 0 )
				kdb(ttype, locr0, 0);
			ptep = (int *)pmap_pte(current_pmap(), va);
			*ptep |= PTE_DIRTY;
			invalidate_TLB();
		    }
		    fixes(locr0);
		    return;
		}
#endif	/* Add a pager */
		if (USERMODE(locr0[PC]))	/* Was this a user trap? */
			break;
		/* Trap from kernel!  Is there a recovery action? */
		recover = P->p_recover;
		if (recover == (vm_offset_t)0) {
			pc = (vm_offset_t)locr0[PC];
			if (pc > (vm_offset_t)&ALLOW_FAULT_START
			     && pc < (vm_offset_t)&ALLOW_FAULT_END) {
				recover = (vm_offset_t) &FAULT_ERROR;
			}
		}
		if (recover != (vm_offset_t)0) {
			locr0[PC] = (int)recover;	/* Return to recovery proc. */
			fixes(locr0);
			return;
		}
		/* Oops! Kernel trap, and no recovery procedure! */
		printf("Kernel: %s ", trap_type[ttype]);
		printf("psr %X ", psr);
		printf("pc %X ", locr0[PC]);
		printf(iswrite ? "write" : "read");
		printf(" aln %D", aln);
		printf(" ins %X", ins);
		printf(" eff addr %X", va);
		printf(" detail %s\n", det_name[detail]);
		kdb(ttype, locr0, 0);
		fixes(locr0);
		break;
	case T_INTRPT:
		do_intr(locr0);
		fixes(locr0);
		return;

	}
	/*
	 *	Trap from user mode.
	 */
	if (ttype != T_SYSCALL)
		fixes(locr0);	/* Possible fixups needed */
	if (P)
		P->p_ar0 = locr0;

	exc_code = 0;
	exc_subcode = 0;
	
	switch (ttype) {

	case T_SYSCALL:
		locr0[PC] += 4;	/* Bump PC past trap opcode */
		syscall(locr0);
		return;
	case T_PAGEFLT:		/* Drop a seg fault on the user. */
		exc_type = EXC_BAD_ACCESS;
		exc_code = result;
		exc_subcode = va;
		break;
	case T_PRIVINFLT:
		exc_type = EXC_BAD_INSTRUCTION;
		exc_code = EXC_i860_PRIV;
		break;
	case T_ARITHTRAP:
		exc_type = EXC_ARITHMETIC;
		break;
	case T_INSFLT:
		exc_type = EXC_BAD_INSTRUCTION;
		exc_code = EXC_i860_IAT;
		break;
	default:
#ifdef	CS_KDB
		(void) kdb_trap(locr0);
#endif	CS_KDB
		return;

	}
#if 0
	process_exception(P, exc_type, exc_code, exc_subcode);
#else
	kdb_trap(locr0);
#endif
	/* Do signal emulation here? */
	
	while( P->p_stat == SFREE )	/* Process should terminate? */
		Switch();

	while ( runrun )
		Switch();		/* Sched in the waiting more urgent proc */

}

/*
 * Called from the trap handler when a system call occurs
 */
/*ARGSUSED*/
syscall(locr0)
	register int *locr0;
{
	unsigned code;
	int pc;
	register int i;				/* known to be r9 below */
	register int	error;
	int		s;
	register struct proc *p;
	struct sysent *callp;
	int opc;
	short	syscode;

	code = locr0[R31];
	pc = locr0[PC];
	P->p_ar0 = locr0;
	P->p_arg = (int *)&locr0[R16];
	P->p_error = 0;
	opc = pc - 4;				/* PC at trap opcode */
	
	if (code >= nsysent)
		callp = &sysent[0];		/* indir (illegal) */
	else {
		callp = &sysent[code];
	}
	if (setjmp(P->p_qsav)) {
		if (error == 0 )
			error = EINTR;
		P->p_eosys = RESTARTSYS;
	}
#ifdef SYSCALLTRACE
	if (syscalltrace) {
#if	CS_LINT
#define	i	ii
#endif	CS_LINT
			register int i;
			char *cp;

			if (code >= nsysent)
				printf("0x%X", code);
			else
				printf("%s", syscallnames[code]);
			cp = "(";
			for (i= 0; i < callp->sy_narg; i++) {
				printf("%s%X", cp, P->p_arg[i]);
				cp = ", ";
			}
			if (i)
				putchar(')', 0);
#if	CS_LINT
#undef	i
#endif	CS_LINT
		}
#endif SYSCALLTRACE
		(*(callp->sy_call))();

		error = P->p_error;
#if SYSCALLTRACE
		if (syscalltrace && 
		    (syscalltrace == P->p_pid || syscalltrace < 0)) {
			if (error)
				printf(" = -1 [%D]\n", error);
			else
				printf(" = %X:%X\n", P->p_rval1, P->p_rval2);
		}
	}
#endif SYSCALLTRACE
	if (P->p_eosys == NORMALRETURN) {
		if (error) {
			locr0[R16] = error;
			locr0[PSR] |= PSR_CC;	/* carry bit set */
		} else {
			locr0[R16] = P->p_rval1;
			locr0[R17] = P->p_rval2;
			locr0[PSR] &= ~PSR_CC;	/* carry clear */
		}
	} else if (P->p_eosys == RESTARTSYS)
		pc = opc;
	/* else if (P->p_eosys == JUSTRETURN) */
		/* nothing to do */
done:
	P->p_error = error;
	
	/* Unix signal emulation goes here. */
	while( P->p_stat == SFREE )	/* Process should terminate */
		Switch();

	while ( runrun )
		Switch();		/* Sched in the waiting more urgent proc */

}
#endif

/*
 * fixes - checking instruction at (fir-4) before returning from the trap 
 *         handler
 *
 * This implements all of the PC fixups specified in the PRM Sections 7.2.3.1
 * through 7.2.3.2
 */
fixes(locr0)
int *locr0;
{
	register int *pc, *opc;
	register unsigned psr, previhi;
	register unsigned long previ, inst;
	register int src1, src2;

	psr = locr0[PSR];		/* Processor Status Register */
	pc = (int *)(locr0[PC]);	/* fir */
	opc = pc-1;			/* fir-4 */
	pc = LE_BE(pc, int *);		/* Convert to big-endian address */
	opc = LE_BE(opc, int *);	/* Convert to big-endian address */
	inst = *pc;	/* the instruction at (fir) */
	
	/*
	 * If opc is on a different page than pc, check to see if opc is
	 * resident.  If not, than skip these tests, because (a) they would fault,
	 * and, (b) the instruction at opc couldn't possibly be a delayed opcode,
	 * because we couldn't have just executed it.
	 */
	if ( trunc_page( pc ) != trunc_page( opc ) && kvtophys(trunc_page(opc)) == 0 )
		goto check_KNF;		/* fir -4 isn't a recently used address. */
	
	previ = *opc;	/* the instruction at (fir-4) */

	switch (previhi = (unsigned)previ >> 26) {
	case 035:	/* bc.t */
	case 037:	/* bnc.t */
	/*
	 * check for a special case of BC.T or BNC.T where:
	 *  1. a source exception has occured.
	 *  2. the instruction at fir is PFGT, PFLE or PFEQ
	 *
	 * In this case we interpret the conditional branch insn, and resume
	 * at it's target.
	 */
		if ((psr & PSR_FT) && /* Floating Point Trap */
		    (locr0[PSV_FSR] & 0x100) &&  /* Source Exception */
		    (inst & 0xfc000000 == 0x48000000)) {
			switch (inst & 0x3f) {
		       /*
			* An unordered compare?  For PFGT and PFEQ, clear CC.
			* For PFLE, set CC.  This matches IEEE defined behavior.
			*
			* If we find a compare opcode, there's no need to check for
			* a possible KNF situation, so just return.
			*/
			case 0x34:	/* PFGT or PFLE */
			    /* Test R bit of insn to determine flavor. */
			    if ( inst & 0x80 ) /* PFLE */
			    	locr0[PSR] |= PSR_CC;
			    else
			    	locr0[PSR] &= ~PSR_CC;
			    /* Emulate the BC.T or BNC.T insn. */
			    locr0[PC] = LE_BE((((inst >> 6) << 4)
			    			+ LE_BE(pc, unsigned)), int);
			    return;
			case 0x35:	/* PFEQ: clear CC */
			    locr0[PSR] &= ~PSR_CC;
			    /* Emulate the BC.T or BNC.T insn. */
			    locr0[PC] = LE_BE((((inst >> 6) << 4)
			    			+ LE_BE(pc, unsigned)), int);
			    return;	
			}
			goto fir_minus_4;
		}
		else
			goto fir_minus_4;
	case 055:	/* bla */
		/* fix up increment already done */
		src1 = (previ >> 11) & 0x1f;
		src2 = (previ >> 21) & 0x1f;
		locr0[R0+src2] -= locr0[R0+src1];
		/* fall through */
	case 020:	/* bri */
	case 032:	/* br */
	case 033:	/* call */
	case 023:	/* core-escape (for calli) */
	fir_minus_4:
		if(previhi == 023 && (previ & 0x1f) != 2)
		        break;  /* core-escape other than calli */
		locr0[PC] = LE_BE(opc, int);	/* Convert to little-endian */
		/*
		 *  test DIM bit in PSR
		 */
		if (psr & PSR_DIM) {
			locr0[PSR] &= ~PSR_DIM;	/* clear DIM in return PSR */
			locr0[PSR] |= PSR_DS;	/* set DS in return PSR */
		}
		break;
	}
	check_KNF: 
	/*
 	 * set KNF bit in psr if:
 	 * 1. All of the following 3 conditions occur -
	 *    1) the trapped instruction is a fp instruction
	 *    2) the processor was in dual instruction mode
    	 *    3) only DAT occured with no other traps
 	 * 2.
    	 *    1) the trap was caused by a source exception of any fp
       	 *       instruction (except when a PFGT, PFLE or PFEQ follows
	 *       a conditional branch instruction).
	 */
       	if (inst & 0xfc000000 == 0x48000000) {		/* inst is fp */
	   if ( (psr & PSR_DIM) &&			/* Dual insn mode */
	   	(locr0[PSR] & 0x1700 == 0))		/* only DAT traps */
		locr0[PSR] |= PSR_KNF;
	   /* We could implement IEEE exceptions here, if we really needed them */
	   if ( (psr & PSR_FT) && 			/* Floating Point Trap */
		(locr0[PSV_FSR] & 0x100) )		/* Source Exception */
		locr0[PSR] |= PSR_KNF;
	}
}

volatile int intrs = 0;
/* int intr[8] = 0; */
int intr[8];	/* Removed bogus initializer.  Not needed and bad syntax, too! */

#define INTRBF	10

struct intrbf {
	int	pc;
	int	reg;
} intrbf[INTRBF];
struct intrbf *intrptr = &intrbf[INTRBF];

int fixpim;
/*
 * Interrupts may come from the host or from special hardware in the Memory Controller
 * chip.
 *
 *	1) Host Interrupt: There is a message queued in the buffers for you.
 *	2) BE interrupt: The host detected an illegal address reference.
 *	3) VBL interrupt: A Vertical Blanking interval has started.
 *	4) VIOBL interrupt: A Video I/O Vertical Blanking interval has started.
 *
 * Once the EPSR INT bit shows clear, we check for a message queued in the inbound data
 * buffer.  If there is an inbound message, we post a Wakeup call on the inbound
 * message queue.
 */
#define INTERRUPT_BITS (NDCSR_INT860 | NDCSR_BE_INT | NDCSR_VBL_INT | NDCSR_VIOVBL_INT)

do_intr(locr0)
int *locr0;
{
	extern volatile int *__ND_csr;
	extern int ki_enable, ui_enable;
	register int i, *pc, inst;
	register short intr_src;
	register int intr_bits;

	pc = (int *)(locr0[PC]);	/* fir */
	inst = *pc;	/* the instruction at (fir) */

	intrs++;
	
	/*
	 * Clear the interrupt(s).
	 */
#if defined(PROTOTYPE)
	intr_bits = *ADDR_NDP_CSR;
	*ADDR_NDP_CSR = (intr_bits & ~(NDCSR_INT860 | NDCSR_VBLANK));
#else /* !PROTOTYPE */
	intr_bits = (*ADDR_ND_CSR0 & INTERRUPT_BITS);
	*ADDR_ND_CSR0 &= ~intr_bits;
#endif
	/*
	 * If the only interrupt is a hardware Bus Error, bomb out to the debugger.
	 */
	if ( intr_bits == NDCSR_BE_INT )
		kdb( T_SEGFLT, locr0, 0 );

	/*
	 * Check the inbound message queue for possible work to do.
	 * Post a Wakeup if appropriate.
	 */
	if ( MSG_QUEUES->ToND.Head != MSG_QUEUES->ToND.Tail )
	{
		Wakeup(  &MSG_QUEUES->ToND );
	}
	if ( MSG_QUEUES->ReplyND.Head != MSG_QUEUES->ReplyND.Tail )
	{
		Wakeup(  &MSG_QUEUES->ReplyND );
	}
	/*
	 * If there is space available on the outbound queue now, wake up the 
	 * appropriate process.
	 */
	if ( MSG_QUEUES->FromND.Head == MSG_QUEUES->FromND.Tail )
		Wakeup(  &MSG_QUEUES->FromND );
	/*
	 * Vertical blanking tasks.
	 */
#if defined(PROTOTYPE)
	if ( intr_bits & NDCSR_VBLANK )
		hardclock(locr0);
#else
	if ( intr_bits & NDCSR_VBL_INT )
		hardclock(locr0);
#endif
	/*
	 * Perform housekeeping, chip bug workarounds.
	 */
	/* fix chip bug? lost PIM */
	if ((locr0[PSR] & PSR_PIM) == 0) {
		fixpim++;
		if (++intrptr >= &intrbf[INTRBF])
			intrptr = intrbf;
		intrptr->pc = locr0[PC];
		intrptr->reg = locr0[PSR];
		locr0[PSR] |= PSR_PIM;
	}
	return;
}




