#ifndef __ND_VAR_H__
#define __ND_VAR_H__	1
#include <sys/kern_return.h>
#if defined(KERNEL)
#include <kern/thread.h>
#endif
#include <kern/lock.h>
#include <next/spl.h>
#include <sys/port.h>
#include <sys/message.h>
#include <nextdev/slot.h>

#include <ND/ND_conio.h>
#include <ND/NDreg.h>
#include <ND/NDmsg.h>

#if defined(KERNEL)
typedef struct ND_port_name_s
{
	struct ND_port_name_s *next;
	port_t	port;
	char *	name;
} ND_port_name;

typedef struct ND_var_s {
	lock_t			lock;
	
	port_t			owner_port;	/* Host ownership token */
	port_t			negotiation_port;
	port_t			kernel_port;	/* Host to ND kernel messages */
	port_t			debug_port;	/* ND Kernel to Host messages */
	port_t			service_port;	/* ND sends on this port */
	port_t			service_replyport;	/* ND receives on this port */
	port_set_name_t		ND_portset;	/* Bag of ports for i860 apps. */
	
	ND_port_name		*portnames;	/* List of known ports */	
	
	int			slot;		/* The logical slot number */
	boolean_t		present;	/* Slot hardware is present */
	boolean_t		attached;	/* This slot is attached */
		
	/* CAUTION - transparent mapping only active in service threads! */
	vm_offset_t		board_addr;	/* Transparent mapped board - 16 Mb */

	vm_offset_t		csr_addr;	/* Mapped in CSR - 32 bytes */
	vm_offset_t		bt_addr;	/* Mapped in BT463 - 32 bytes */
	vm_offset_t		fb_addr;	/* Mapped in frame buffer - 4 Mb */
	vm_offset_t		globals_addr;	/* Mapped in ND globals - 64 Kbytes */
	vm_offset_t		slot_addr;	/* Mapped in NBIC regs - 8 bytes */
	
	thread_t		thread;
	thread_t		server_thread;
	thread_t		reply_thread;
	task_t			pager_task;	/* Task space used for backing store */
	
	vm_map_t		map_self;	/* Our VM map, for out of line msgs */
	vm_map_t		map_pager;	/* Backing store VM map */
	
	simple_lock_data_t	flags_lock;
	int			flags;
	
	int			lockno;		/* Lock token for ND queues */
	int			ev_token;	/* Screen registry token from ev_register_screen() */
	
	int			pager_flags;	/* Flags for ND pager support sys. */
	vm_offset_t		npg_vaddr;	/* Task addr for cached pages */
	vm_offset_t		npg_at;		/* Page cache in our task space */
	vm_size_t		npg_size;	/* Size of cached pages */
	int			npg_hits;
	int			npg_misses;
} ND_var_t;


#define ND_FLAG_SLEEPING		0x00000001
#define ND_REPLY_THREAD_RUNNING		0x00000002
#define ND_SERVER_THREAD_RUNNING	0x00000004
#define ND_STOP_REPLY_THREAD		0x00000008
#define ND_STOP_SERVER_THREAD		0x00000010
#define ND_REPLY_THREAD_PAUSED		0x00000020
#define ND_SERVER_THREAD_PAUSED		0x00000040

#define ND_INTR_PENDING			0x40000000
#define ND_MSG_SYSTEM_RESET		0x80000000

/* Pager support flags */
#define ND_PAGEAHEAD			0x00000001

/* Max size a reply message (from kernel to i860 or host) can be, in bytes. */
#define ND_MAX_REPLYSIZE		512

kern_return_t set_owner (
	void		*arg,
	port_t		owner_port,
	port_t		*negotiation_port);

	
kern_return_t console_input (
	void		*arg,
	char *		data,
	int		length);
	
kern_return_t map_hardware( void *arg, port_t thread_port, int *mapped_addr );
kern_return_t map_anything( void *arg, port_t thread_port, int *map_addr, int *map_mask, int cache );
kern_return_t map_disable( void *arg, port_t thread_port);


kern_return_t RegisterThyself(
		void	*device_port,
		int	NXSDevice,
		int	Min_X,
		int	Max_X,
		int	Min_Y,
		int	Max_Y );

void	ND_interrupt_disable(int slotid);
void	ND_reply_msg(void *arg);
int	ND_intr(void);
void	ND_init_hardware( ND_var_t * );
void	ND_arm_interrupts( ND_var_t * );
void	ND_disarm_interrupts( ND_var_t * );
void	ND_LamportLock( ND_var_t *, NDQueue *);
void	ND_start_services( ND_var_t * );
void	ND_stop_services( ND_var_t * );
void	ND_reply_server_loop( void );
void	ND_msg_server_loop ( void );

kern_return_t NDPortToMap( port_t, vm_map_t * );
/* Macro to release a Lamport lock. */
#define ND_LamportUnlock(q) \
	((q)->Lock_y = LOCK_UNDEF, (q)->Lock_b[LOCK_NUM] = FALSE)


/* Make sure that the unit describes a valid slot in the current machine */
#define UNIT_INVALID(u)	((u) < 0 || (u) > (2*SLOTCOUNT) || ((u) & 1))
#define UNIT_TO_INDEX(u)	((u)>>1)

#define ND_mask_register(s) ((int *)(ND_var[(s)].slot_addr + 4))
#define ND_intr_register(s) ((int *)(ND_var[(s)].slot_addr))
#define SLOT_MASK_BIT	0x80000000	/* GAD[7] as seen by the 68030 */
#define SLOT_INTR_BIT	0x80000000

#define	spl_ND()	spl5()		/* Our interrupt level */

#if defined(PROTOTYPE)
/* Memory mapping info for the prototype hardware */
#define ND_BOARD_PHYS_ADDR(s)		(0x0b000000 | (s)<<29)
#define ND_CSR_PHYS_ADDR(s)		(0x0b000000 | (s)<<29)
#define ND_FB_PHYS_ADDR(s)		(0x0b400000 | (s)<<29)
#define ND_DRAM_PHYS_ADDR(s)		(0x0b800000 | (s)<<29)
#define ND_GLOBALS_PHYS_ADDR(s)		(0x0bFF0000 | (s)<<29)
#define ND_SLOT_PHYS_ADDR(s)		(0xF0FFFFE8 | (s)<<25)
#define ND_BOARD_SIZE			0x01000000
#define ND_CSR_SIZE			32
#define ND_FB_SIZE			0x00400000
#define ND_DRAM_SIZE			0x00800000
#define ND_GLOBALS_SIZE			0x00010000
#define ND_SLOT_SIZE			8

/* Convert an i860 phys address to a mapped address */
#define ND_BOARD_ADDR_MASK		0x00FFFFFF	/* 16 Mbytes significant... */
#define CONVERT_PADDR(sp, addr) \
	(((sp)->board_addr) | ((addr) & ND_BOARD_ADDR_MASK))
#else /* ! PROTOTYPE */
/* Memory mapping info for the production hardware */
#define ND_BOARD_PHYS_ADDR(s)		(0x00000000 | (s)<<29)
#define ND_CSR_PHYS_ADDR(s)		(0x0F800000 | (s)<<29) /* MC Regs */
#define ND_FB_PHYS_ADDR(s)		(0x0E000000 | (s)<<29)
#define ND_DRAM_PHYS_ADDR(s)		(0x08000000 | (s)<<29)
#define ND_GLOBALS_PHYS_ADDR(s)		(0x080F0000 | (s)<<29)	/* Last 64K in 1st Mb*/
#define ND_BT_PHYS_ADDR(s)		(0xF0200000 | (s)<<25)	/* BT463 on localbus */
#define ND_SLOT_PHYS_ADDR(s)		(0xF0FFFFE8 | (s)<<25)
#define ND_BOARD_SIZE			0x10000000
#define ND_CSR_SIZE			0x4000
#define ND_BT_SIZE			32
#define ND_FB_SIZE			0x00400000
#define ND_DRAM_SIZE			0x04000000		/* 64 Mbytes */
#define ND_GLOBALS_SIZE			0x00010000		/* 64 Kb */
#define ND_SLOT_SIZE			8

/* Convert an i860 phys address to a mapped address */
#define ND_BOARD_ADDR_MASK		0x0FFFFFFF	/* 256 Mbytes significant... */
#define CONVERT_PADDR(sp, addr) \
	(((sp)->board_addr) | ((addr) & ND_BOARD_ADDR_MASK))
#endif /* ! PROTOTYPE */
#endif /* KERNEL */
#endif /* __ND_VAR_H__ */




