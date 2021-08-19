/*
 *	NDlib.h
 *
 *	Something resembling the defs file for the finished ND board.
 */
#if ! defined(__NDLIB_H__)
#define __NDLIB_H__
#include <mach_types.h>
#include <sys/port.h>
#include <sys/message.h>
#include <nextdev/slot.h>

#include "ND/NDreg.h"
#include "ND/NDmsg.h"

/* Mach driver name and path name for the relocatable */
#define ND_SERV_RELOC "/usr/lib/kern_loader/NextDimension/NextDimension_reloc"
#define ND_SERV_NAME "NextDimension"

/*
 *	WARNING: The contents of these structures are subject to change without notice.
 *	The names are kept more or less in sync with the driver ND_var.h ND_var_t type.
 *
 */
#if !defined(i860) && !defined(KERNEL)	/* Stuff for host side only */

#include <sys/loader.h>		/* Defn of struct mach_header */

typedef	struct _ND_var_t_
{
	port_t		dev_port;	/* Host to Driver messages */
	port_t		owner_port;	/* Host ownership token */
	port_t		negotiation_port;
	port_t		kernel_port;	/* Host to ND kernel messages */
	
	port_t		local_port;	/* Cache for user's reply port. */
	port_t		NDlocal_port;	/* Cached kernel equiv. for local_port */
	port_t		NDkernel_port;	/* Cached kernel equiv. for kernel_port */
	
	vm_address_t	csr_addr;	/* Mapped in CSR - 32 bytes */
#if ! defined(PROTOTYPE)
	vm_address_t	bt_addr;	/* Mapped in Bt463 CSR - 32 bytes */
#endif
	vm_address_t	fb_addr;	/* Mapped in frame buffer - 4 Mb */
	vm_address_t	globals_addr;	/* Mapped in ND globals - 64 Kbytes */
	vm_address_t	dram_addr;	/* Mapped in ND DRAM - 8 Mbytes */
	vm_address_t	host_to_ND_addr;	/* Mapped in ND queue - 64 Kbytes */
	
	vm_size_t	csr_size;
	vm_size_t	fb_size;
	vm_size_t	globals_size;
	vm_size_t	dram_size;
	vm_size_t	host_to_ND_size;

	int		slot;		/* The hardware slot owned by this struct */
	
	int		t_flag;		/* Task state flags for the i860 and driver */
} ND_var_t;

typedef struct _ND_var_t_	*ND_var_p;

#define T_FLAG_i860_EXIT	1	/* ND Board processor has shut down! */

#define NUMHANDLES		(SLOTCOUNT-1)	/* Max number of boards supported */
#define SlotToHandleIndex(s)	(((s)>>1) - 1)	/* Map a hardware slot to a Handle */
#define SLOT_INVALID(u)	((u) < 0 || (u) > (2*SLOTCOUNT) || ((u) & 1))


/*
 * Published interface functions.  Anyone can call these.
 */
kern_return_t	ND_Open( port_t, int );
void		ND_Close( port_t, int );
int		NDSetGamma( int, double );
int		NDSetGamma3( int, double, double, double );
kern_return_t	ND_BootKernel( port_t, int, char *);
kern_return_t	ND_BootKernelFromSect(port_t,int,struct mach_header *,char *,char *);
void		ND_DefaultServer( port_t, port_t );
boolean_t	ND_Server( msg_header_t *, msg_header_t * );

/*
 * Internal functions.  These should only be called within NDlib.
 */
msg_return_t	_NDmsg_send_(ND_var_p, msg_header_t *, msg_option_t , msg_timeout_t);
int	NDLoadCode( ND_var_p, char *);
int	NDLoadCodeFromSect( ND_var_p, struct mach_header *, char *, char *);
int	NDSetTrapVector( ND_var_p, unsigned long );

void	ND_LamportLock( NDQueue * );

/*
 * Internal macros.  These should only be called within NDlib.
 */
#define ND_LamportUnlock(q) \
	((q)->Lock_y = LOCK_UNDEF, (q)->Lock_b[LOCK_NUM] = FALSE)

/*
 * Internal structures.  These should only be used within NDlib.
 */
extern ND_var_p	_ND_Handles_[];

#endif /* ! i860 && ! KERNEL */

#endif


