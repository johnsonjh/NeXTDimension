/* COFF medicine for system V.
   Copyright (C) 1988 Free Software Foundation, Inc.

		       NO WARRANTY

  BECAUSE THIS PROGRAM IS LICENSED FREE OF CHARGE, WE PROVIDE ABSOLUTELY
NO WARRANTY, TO THE EXTENT PERMITTED BY APPLICABLE STATE LAW.  EXCEPT
WHEN OTHERWISE STATED IN WRITING, FREE SOFTWARE FOUNDATION, INC,
RICHARD M. STALLMAN AND/OR OTHER PARTIES PROVIDE THIS PROGRAM "AS IS"
WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
FITNESS FOR A PARTICULAR PURPOSE.  THE ENTIRE RISK AS TO THE QUALITY
AND PERFORMANCE OF THE PROGRAM IS WITH YOU.  SHOULD THE PROGRAM PROVE
DEFECTIVE, YOU ASSUME THE COST OF ALL NECESSARY SERVICING, REPAIR OR
CORRECTION.

 IN NO EVENT UNLESS REQUIRED BY APPLICABLE LAW WILL RICHARD M.
STALLMAN, THE FREE SOFTWARE FOUNDATION, INC., AND/OR ANY OTHER PARTY
WHO MAY MODIFY AND REDISTRIBUTE THIS PROGRAM AS PERMITTED BELOW, BE
LIABLE TO YOU FOR DAMAGES, INCLUDING ANY LOST PROFITS, LOST MONIES, OR
OTHER SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES ARISING OUT OF THE
USE OR INABILITY TO USE (INCLUDING BUT NOT LIMITED TO LOSS OF DATA OR
DATA BEING RENDERED INACCURATE OR LOSSES SUSTAINED BY THIRD PARTIES OR
A FAILURE OF THE PROGRAM TO OPERATE WITH ANY OTHER PROGRAMS) THIS
PROGRAM, EVEN IF YOU HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH
DAMAGES, OR FOR ANY CLAIM BY ANY OTHER PARTY.

		GENERAL PUBLIC LICENSE TO COPY

  1. You may copy and distribute verbatim copies of this source file
as you receive it, in any medium, provided that you conspicuously and
appropriately publish on each copy a valid copyright notice "Copyright
 (C) 1988 Free Software Foundation, Inc."; and include following the
copyright notice a verbatim copy of the above disclaimer of warranty
and of this License.  You may charge a distribution fee for the
physical act of transferring a copy.

  2. You may modify your copy or copies of this source file or
any portion of it, and copy and distribute such modifications under
the terms of Paragraph 1 above, provided that you also do the following:

    a) cause the modified files to carry prominent notices stating
    that you changed the files and the date of any change; and

    b) cause the whole of any work that you distribute or publish,
    that in whole or in part contains or is a derivative of this
    program or any part thereof, to be licensed at no charge to all
    third parties on terms identical to those contained in this
    License Agreement (except that you may choose to grant more extensive
    warranty protection to some or all third parties, at your option).

    c) You may charge a distribution fee for the physical act of
    transferring a copy, and you may at your option offer warranty
    protection in exchange for a fee.

Mere aggregation of another unrelated program with this program (or its
derivative) on a volume of a storage or distribution medium does not bring
the other program under the scope of these terms.

  3. You may copy and distribute this program or any portion of it in
compiled, executable or object code form under the terms of Paragraphs
1 and 2 above provided that you do the following:

    a) accompany it with the complete corresponding machine-readable
    source code, which must be distributed under the terms of
    Paragraphs 1 and 2 above; or,

    b) accompany it with a written offer, valid for at least three
    years, to give any third party free (except for a nominal
    shipping charge) a complete machine-readable copy of the
    corresponding source code, to be distributed under the terms of
    Paragraphs 1 and 2 above; or,

    c) accompany it with the information you received as to where the
    corresponding source code may be obtained.  (This alternative is
    allowed only for noncommercial distribution and only if you
    received the program in object code or executable form alone.)

For an executable file, complete source code means all the source code for
all modules it contains; but, as a special exception, it need not include
source code for modules which are standard libraries that accompany the
operating system on which the executable file runs.

  4. You may not copy, sublicense, distribute or transfer this program
except as expressly provided under this License Agreement.  Any attempt
otherwise to copy, sublicense, distribute or transfer this program is void and
your rights to use the program under this License agreement shall be
automatically terminated.  However, parties who have received computer
software programs from you with this License Agreement will not have
their licenses terminated so long as such parties remain in full compliance.

  5. If you wish to incorporate parts of this program into other free
programs whose distribution conditions are different, write to the Free
Software Foundation at 675 Mass Ave, Cambridge, MA 02139.  We have not yet
worked out a simple rule that can be stated here, but we will often permit
this.  We will be guided by the two goals of preserving the free status of
all derivatives our free software and of promoting the sharing and reuse of
software.


In other words, you are welcome to use, share and improve this program.
You are forbidden to forbid anyone else to use, share and improve
what you give them.   Help stamp out software-hoarding!  */

/* You must define the symbols SEGSIZ, COFFMAGIC
   and IMPURE_COFFMAGIC appropriately for your machine.  */

#ifndef __GNU_ROBOTUSSIN__

#define __GNU_ROBOTUSSIN__

#include <filehdr.h>
#include <aouthdr.h>
#include <scnhdr.h>

#define	NBPG NBPP
#include "getpagesize.h"

/* In ld, N_BADMAG only uses 3 values. Elsewhere, it needs 4. */
#ifndef	__LD__
#define	__LD__ 0
#endif

union all_headers {
  struct exec bsd_outheader;
  struct hdr {
    struct filehdr	filehdr;
    struct aouthdr	aouthdr;
    struct scnhdr	scnhdr[3];	/* de toutes facons, on n'en connait pas plus */
  } coffhdr;
};
#define	txtscn coffhdr.scnhdr[0]
#define	datscn coffhdr.scnhdr[1]
#define	bssscn coffhdr.scnhdr[2]
/* only with ANSI cpp; standard cpp loses, because of macro recursion: cf Makefile */
#define	a_magic bsd_outheader.a_magic
#define	a_text bsd_outheader.a_text
#define	a_data bsd_outheader.a_data
#define	a_bss bsd_outheader.a_bss
#define	a_syms bsd_outheader.a_syms
#define	a_entry bsd_outheader.a_entry
#define	a_trsize bsd_outheader.a_trsize
#define	a_drsize bsd_outheader.a_drsize

#define	IS_COFF(x) ((x) == COFFMAGIC || (x) == IMPURE_COFFMAGIC)
#define	FILEMAGIC(x) (*((union all_headers *)&(x))).coffhdr.filehdr.f_magic
#define	COFF_HEADER(x) IS_COFF(FILEMAGIC(x))

/* Now redefine the N_... macros that access the header of a BSD-format file
   so that they can access either that or a BSD-within-COFF file
   whose header is stored in a COFF-like fashion.
   Use the _N_... macros, whose definitions are initially the same,
   to handle the BSD case, so that the only case we need define here
   is the BSD-within-COFF case.  */

#undef	N_BADMAG
#define	N_BADMAG(x) \
	(!((!(__LD__) && COFF_HEADER(x)) \
	 || (*((union all_headers *)&(x))).a_magic==OMAGIC \
	 || (*((union all_headers *)&(x))).a_magic==NMAGIC \
	 || (*((union all_headers *)&(x))).a_magic==ZMAGIC))

#undef N_TXTOFF
#define N_TXTOFF(x)	(COFF_HEADER (x) ? sizeof (struct hdr) : _N_TXTOFF (x))

/* Handle N_SYMOFF for a COFF file.  */
#define	COFF_SYMOFF(x)	(N_TXTOFF (x) + (x).txtscn.s_size + (x).datscn.s_size \
			 + (((x).txtscn.s_nreloc + (x).datscn.s_nreloc)	      \
			    * sizeof (struct relocation_info)))
#undef N_SYMOFF
#define	N_SYMOFF(x)	(COFF_HEADER (x) ? COFF_SYMOFF (x) : _N_SYMOFF (x))

#undef	N_STROFF
#define	N_STROFF(x) 			\
 (N_SYMOFF (x)				\
  + (COFF_HEADER(x)			\
     ? ((x).coffhdr.filehdr.f_nsyms * sizeof (struct nlist))	\
     : (x).a_syms))


#define PG_TXTADDR	(SEGSIZ + sizeof (struct hdr))
#define WR_TXTADDR	SEGSIZ
#define	COFF_TXTADDR(x)	(FILEMAGIC(x) == COFFMAGIC ? PG_TXTADDR : WR_TXTADDR)
#define N_TXTADDR(x)	(!COFF_HEADER (x) ? _N_TXTADDR (x) : COFF_TXTADDR (x))

#define	COFF_TXTEND(x)	(COFF_TXTADDR (x) + (x).txtscn.s_size)

/* This macro will have to be different on other machines.  */
#define COFF_DATADDR(x)				\
  (FILEMAGIC (x) == COFFMAGIC			\
   ? ((COFF_TXTEND (x) + SEGSIZ) & -SEGSIZ)	\
   : COFF_TXTEND (x))

#undef N_DATADDR
#define N_DATADDR(x)	(COFF_HEADER (x) ? COFF_DATADDR (x) : _N_DATADDR (x))

#endif /* __GNU_ROBOTUSSIN__ */
