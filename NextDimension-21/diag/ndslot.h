/* 
 * HISTORY
 * 27-May-89  Avadis Tevanian, Jr. (avie) at NeXT, Inc.
 *	Created.
 */

#ifndef	_SLOT_H_
#define	_SLOT_H_

#define SLOTSIZE	0x10000000		/* each physical slot is
						   really two of these */
#define SSLOTSIZE	0x01000000
#define SSLOTBASE	0xF0000000

#define SLOTCOUNT	4

#define	SLOTIOCGADDR	_IOR('s', 0, int)	/* get address of slot space */
#define	SLOTIOCGADDR_CI	_IOR('s', 1, int)	/* address of slot space, cache off */
#define	SLOTIOCDISABLE	_IO('s', 3)		/* disable translation */
#define	SLOTIOCSTOFWD	_IO('s', 4)		/* Enable NBIC store/forward */
#define	SLOTIOCNOSTOFWD	_IO('s', 5)		/* Disable NBIC store/forward */

/*
 * The following two ioctls take a packed pair of 8 bit address/mask fields
 * to be loaded into TT1, and return the user process address holding the
 * mapped address ranges.
 */
#define	SLOTIOCMAPGADDR	_IOWR('s', 6, int)	   /* map and get addr */
#define	SLOTIOCMAPGADDR_CI	_IOWR('s', 7, int) /* map and get addr, cache off */

/*
 * Form a packed address/mask value from a pair of 32 bit address/mask values.
 */
#define FORM_MAPGADDR(addr,mask)	((((addr)>>16)&0xFF00)|(((mask)>>24)&0xFF))
#ifdef KERNEL
#define UNPACK_ADDR(val)		((((val)>>8)&0xFF)<<24)
#define UNPACK_MASK(val)		((val)&0xFF)
#endif /* KERNEL */
#endif	/* _SLOT_H_  */
