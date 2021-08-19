
#ifndef __ND_ID__
#define __ND_ID__

/*
 * Message ID values used within the NextDimension subsystem.
 * These values get used in MiG files, so no offset math can be used here.
 */
#define ND_MSG_ID_START		0
#define ND_CONIO_START		ND_MSG_ID_START
#define	ND_KERNEL_SUBSYS_START	100
#define	ND_KERNEL_SUBSYS_END	200
#define ND_MSG_ID_END		ND_KERNEL_SUBSYS_END

#endif __ND_ID__

