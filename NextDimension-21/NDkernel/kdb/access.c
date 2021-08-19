/* 
 * Mach Operating System
 * Copyright (c) 1987 Carnegie-Mellon University
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 */
/*
 ****************************************************************
 * HISTORY
 *  3-Oct-87  Michael Young (mwyoung) at Carnegie-Mellon University
 *	De-linted.
 *
 * 16-May-86 David Golub (dbg) at Carnegie-Mellon University
 *	Support for multiple-process debugging
 *
 ****************************************************************
 */

/*
 * Adb: access data in file/process address space.
 *
 * The routines in this file access referenced data using
 * the maps to access files, ptrace to access subprocesses,
 * or the system page tables when debugging the kernel,
 * to translate virtual to physical addresses.
 */



#include "defs.h"


map		txtmap;
map		datmap;
INT		wtflag;
string_t	errflg;
INT		errno;

INT		pid;

/*
 * Primitives: put a value in a space, get a value from a space
 * and get a word or byte not returning if an error occurred.
 */
put(addr, space, value)
vm_offset_t addr;
{
	(void) access(WT, addr, space, value);
}

unsigned int
get(addr, space)
vm_offset_t addr;
{
	return (access(RD, addr, space, 0));
};

unsigned int
chkget(addr, space)
vm_offset_t addr;
{
	unsigned int w = get(addr, space);
	chkerr();
	return(w);
}

unsigned int
bchkget(addr, space)
vm_offset_t addr;
{
	return(chkget(addr, space) & 0377);
}

/*
 * Read/write according to mode at address addr in i/d space.
 * Value is quantity to be written, if write.
 *
 * This routine decides whether to get the data from the subprocess
 * address space with ptrace, or to get it from the files being
 * debugged.
 *
 * When the kernel is being debugged with the -k flag we interpret
 * the system page tables for data space, mapping p0 and p1 addresses
 * relative to the ``current'' process (as specified by its p_addr in
 * <p) and mapping system space addresses through the system page tables.
 */
access(mode, addr, space, value)
int mode, space, value;
vm_offset_t addr;
{
	int rd = mode == RD;
	int file, w;

	if ( space & ISP )	/* Insn space is little-endian. Correct the addr. */
		addr ^= 4;
	if (space == NSP)
		return(0);
	w = 0;
	if (mode==WT && wtflag==0)
		error("not in write mode");
	if (!chkmap(&addr, space))
		return (0);
	file = (space&DSP) ? datmap.ufd : txtmap.ufd;
#if 0
	if (kernel && space == DSP) {
		addr = vtophys(addr);
		if (addr < 0)
			return (0);
	}
#else
	if ( addr < VM_MIN_KERNEL_ADDRESS && (get_dirbase() & 1) != 0 && 
	    pmap_translation_valid(current_pmap(),addr,VM_PROT_READ) != KERN_SUCCESS )
		return 0;
#endif
	if (physrw(file, addr, rd ? &w : &value, rd) < 0)
		rwerr(space);
	return (w);
}


rwerr(space)
int space;
{

	if (space & DSP)
		errflg = "data address not found";
	else
		errflg = "text address not found";
}

physrw(file, addr, aw, rd)
vm_offset_t addr;
int *aw, rd;
{
	if (rd)
		kdbrlong(addr, aw);
	else
		kdbwlong(addr, aw);
	return (0);
}

chkmap(addr,space)
register long	*addr;
register INT		space;
{
	register map_t amap;
	amap=((space&DSP?&datmap:&txtmap));
	if ( space&STAR || !within(*addr,amap->b1,amap->e1) ) {
		if ( within(*addr,amap->b2,amap->e2) ) {
			*addr += (amap->f2)-(amap->b2);
		} else {
			rwerr(space);
			return(0);
		}
	} else {
		*addr += (amap->f1)-(amap->b1);
	}
	return(1);
}

within(addr,lbd,ubd)
unsigned int addr, lbd, ubd;
{
	return(addr>=lbd && addr<ubd);
}

