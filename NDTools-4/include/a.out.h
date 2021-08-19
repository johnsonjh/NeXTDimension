/* Copyright (C) 1987 Free Software Foundation, Inc.

This file is part of Gas, the GNU Assembler.

The GNU assembler is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY.  No author or distributor
accepts responsibility to anyone for the consequences of using it
or for whether it serves any particular purpose or works at all,
unless he says so in writing.  Refer to the GNU Assembler General
Public License for full details.

Everyone is granted permission to copy, modify and redistribute
the GNU Assembler, but only under the conditions described in the
GNU Assembler General Public License.  A copy of this license is
supposed to have been given to you along with the GNU Assembler
so you can know your rights and responsibilities.  It should be
in a file named COPYING.  Among other things, the copyright
notice and this notice must be preserved on all copies.  */

/* JF I'm not sure where this file came from.  I put the permit.text message in
   it anyway.  This file came to me as part of the original VAX assembler */

#define OMAGIC 0407
#define NMAGIC 0410
#define ZMAGIC 0413

struct exec {
	long	a_magic;			/* number identifies as .o file and gives type of such. */
	unsigned a_text;		/* length of text, in bytes */
	unsigned a_data;		/* length of data, in bytes */
	unsigned a_bss;		/* length of uninitialized data area for file, in bytes */
	unsigned a_syms;		/* length of symbol table data in file, in bytes */
	unsigned a_entry;		/* start address */
	unsigned a_trsize;		/* length of relocation info for text, in bytes */
	unsigned a_drsize;		/* length of relocation info for data, in bytes */
};

#define N_BADMAG(x) \
 (((x).a_magic)!=OMAGIC && ((x).a_magic)!=NMAGIC && ((x).a_magic)!=ZMAGIC)

#define N_TXTOFF(x) \
 ((x).a_magic == ZMAGIC ? 1024 : sizeof(struct exec))

#define N_SYMOFF(x) \
 (N_TXTOFF(x) + (x).a_text + (x).a_data + (x).a_trsize + (x).a_drsize)

#define N_STROFF(x) \
 (N_SYMOFF(x) + (x).a_syms)

struct nlist {
	union {
		char	*n_name;
		struct nlist *n_next;
		long	n_strx;
	} n_un;
	char	n_type;
	char	n_sect;	/* Mach-0 section */
	short	n_desc;
	unsigned n_value;
};

/* 
 * If the type is N_INDR then the symbol is defined to be the same as another
 * symbol.  In this case the n_value field is an index into the string table
 * of the other symbol's name.  When the other symbol is defined then they both
 * take on the defined type and value.
 */

#define N_UNDF	0
#define N_ABS	2
#define N_TEXT	4
#define N_DATA	6
#define N_BSS	8
#define N_SECT	0xE
#define	N_INDR	0xA
#define N_FN	0x1F		
/*
 * If the type is N_SECT then the n_sect field contains an ordinal of the
 * section the symbol is defined in.  The sections are numbered from 1 and 
 * refer to sections in order they appear in the load commands for the file
 * they are in.  This means the same ordinal may very well refer to different
 * sections in different files.
 *
 * The n_value field for all symbol table entries (including N_STAB's) gets
 * updated by the link editor based on the value of it's n_sect field and where
 * the section n_sect references gets relocated.  If the value of the n_sect 
 * field is NO_SECT then it's n_value field is not changed by the link editor.
 */
#define	NO_SECT		0	/* symbol is not in any section */
#define MAX_SECT	255	/* 1 thru 255 inclusive */

#define N_EXT 1
#define N_TYPE 0x1E
#define N_STAB 0xE0

#if defined(I860)
#include <i860.h>
#else
struct relocation_info {
	int	 r_address;
	unsigned r_symbolnum:24,
		 r_pcrel:1,
		 r_length:2,
		 r_extern:1,
		 r_bsr:1, /* OVE: used on ns32k based systems, if you want */
		 r_disp:1, /* OVE: used on ns32k based systems, if you want */
		 nuthin:2;
};
#endif
#define	R_ABS	0		/* absolute relocation type for Mach-O files */

