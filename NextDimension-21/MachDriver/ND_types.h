
#ifndef __ND_TYPES__
#define __ND_TYPES__

#include <mach_types.h>

#define ND_BAD_PORT	120
#define ND_PORT_BUSY	121
#define ND_NO_BOARD	122

typedef char String256[256];
typedef	char ROMData[1024];
/*
 * Defs for paging system.  These describe lists of pages to move between DRAM and
 * the backing store.
 */
typedef struct
{
	vm_address_t	vaddr;
	vm_address_t	paddr;
	vm_size_t	size;
} vm_movepage_t;

typedef vm_movepage_t *vm_movepages_t;

#endif __ND_TYPES__

