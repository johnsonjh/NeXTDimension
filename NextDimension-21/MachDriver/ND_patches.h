/*
 *	This include file contains code to work around problems and inconsistencies
 *	with other include files shipped as part of the standard release.
 */
#if ! defined(__ND_PATCHES__)
#define __ND_PATCHES__

#if defined(WARP1_BOGUS_INCLUDE_FILE_WORKAROUND)
#import <sys/features.h>
#undef MACH_ASSERT
#undef MACH_LDEBUG
#define MACH_ASSERT	0	/* These were erroneously defined as 1 */
#define MACH_LDEBUG	0
#endif /* WARP1_BOGUS_INCLUDE_FILE_WORKAROUND */

#endif
