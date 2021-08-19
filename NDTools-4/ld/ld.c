#define PAGEZERO_PROTECT /* if on result must run on a 0.81 or better kernel */
#ifndef CROSS
#define CROSS	/* Cross compilation unlinks */
#endif
#ifdef NeXT	/* mach object file only with -DNeXT */
#define MACH_O
#define MACH_SHLIB
#endif NeXT
#undef AOUT_SHLIB /* do not turn on, code suffering from some code neglict */
#if defined(AOUT_SHLIB) || defined(MACH_SHLIB)
#define SHLIB
#endif
#define SYM_ALIAS
#define NO_SYMSEGS
/* Name that loader! */
#if defined(CROSS)
#if defined(I860)
#define LOADER	"ld860"
#endif
#endif /* CROSS */
#if ! defined(LOADER)
#define LOADER	"ld"
#endif
/* Linker `ld' for GNU
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
as you receive it, in any medium, provided that you conspicuously
and appropriately publish on each copy a valid copyright notice
"Copyright (C) 1988 Free Software Foundation, Inc.", and include
following the copyright notice a verbatim copy of the above disclaimer
of warranty and of this License.

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

  3. You may copy and distribute this program (or a portion or derivative
of it, under Paragraph 2) in object code or executable form under the terms
of Paragraphs 1 and 2 above provided that you also do one of the following:

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
all derivatives of our free software and of promoting the sharing and reuse of
software.

 In other words, you are welcome to use, share and improve this program.
 You are forbidden to forbid anyone else to use, share and improve
 what you give them.   Help stamp out software-hoarding!  */

/* Written by Richard Stallman with some help from Eric Albert.  */

/* define this if on a system 
 where the names etext, edata and end should not have underscores.  */
/* #define nounderscore 1 */

#include <a.out.h>
#define LIBMAGIC 0443
#include <ar.h>
#include <stdio.h>
#include <sys/types.h>
#include <strings.h>
#include <sys/stat.h>
#ifndef NO_SYMSEGS
#include "symseg.h"
#endif
#include <sys/file.h>
#include <sys/param.h>
#include <varargs.h>
#ifdef MACH_O
#include <sys/loader.h>
#endif MACH_O
#include <ldsyms.h>
#include <sys/machine.h>
#if defined(NeXT) && ! defined(I860)
#define MACHINE_CPU_TYPE	CPU_TYPE_MC68030
#define MACHINE_CPU_SUBTYPE	CPU_SUBTYPE_NeXT
#endif
#if defined(NeXT) && defined(I860)
#define MACHINE_CPU_TYPE	CPU_TYPE_I860
#define MACHINE_CPU_SUBTYPE	CPU_SUBTYPE_BIG_ENDIAN
#endif

/*
 * Macros which take exec structures as arguments and tell where the
 * various pieces will be loaded.
 */

#ifdef NeXT
/* These should use the pagesize system call, but can't while we are cross
   compiling */
#if defined(I860)
#define PAGSIZ 0x1000
#define SEGSIZ 0x1000
#else
#define PAGSIZ 0x2000
#define SEGSIZ 0x20000
#endif

#define N_PAGSIZ(x) (PAGSIZ)
#define N_SEGSIZ(x) (SEGSIZ)

#define N_TXTADDR(x)  (((x).a_magic==OMAGIC) ? 0 : N_PAGSIZ(x))

#define N_DATADDR(x) \
	(((x).a_magic==OMAGIC)? (N_TXTADDR(x)+(x).a_text) \
	: (N_SEGSIZ(x)+((N_TXTADDR(x)+(x).a_text-1) & ~(N_SEGSIZ(x)-1))))

#define N_BSSADDR(x)  (N_DATADDR(x)+(x).a_data)
#endif NeXT


#define min(a,b) ((a) < (b) ? (a) : (b))

/* Size of a page; obtained from the operating system.  */

int page_size;

/* Symbol table */
/* flag to indicate if the symbol types of ld created symbols are to be mach
   symbols, N_SECT with n_sect set, or 4.3bsd symbol types, N_TEXT, etc. This
   gets changed by the -K flag. */
long mach_syms = 1;

/* Global symbol data is recorded in these structures,
   one for each global symbol.
   They are found via hashing in 'symtab', which points to a vector of buckets.
   Each bucket is a chain of these structures through the link field.  */

typedef
  struct glosym
    {
      /* Pointer to next symbol in this symbol's hash bucket.  */
      struct glosym *link;
      /* Name of this symbol.  */
      char *name;
      /* Value of this symbol as a global symbol.  */
      long value;
      /* Chain of external 'nlist's in files for this symbol, both defs and refs.  */
      struct nlist *refs;
      /* Nonzero means definitions of this symbol as common have been seen,
	 and the value here is the largest size specified by any of them.  */
      int max_common_size;
      /* For relocatable_output, records the index of this global sym in the
	 symbol table to be written, with the first global sym given index 0.  */
      int def_count;
      /* SCS: Nonzero is the index (+1) of the common file table used to record
	 file names which define common syms (for gdb lazy syms) */
      struct file_entry *common_file_entry;
#ifdef SYM_ALIAS
      /* Nonzero means that this symbol is to be aliased to this name */
      char *alias_name;
#endif SYM_ALIAS
      /* Nonzero means a definition of this global symbol is known to exist.
	 Library members should not be loaded on its account.  A value higher
	 than 1 is the n_type code for the symbol's definition.  */
      char defined;
      /* The section the symbol is defined in if defined is N_SECT */
      char sect;
      /* Nonzero means a reference to this global symbol has been seen
	 in a file that is surely being loaded. */
      char referenced;
      /* Nonzero means print a message at all refs or defs of this symbol */
      char trace;
    }
  symbol;

/* Number of buckets in symbol hash table */
#define	TABSIZE	1009

/* The symbol hash table: a vector of TABSIZE pointers to struct glosym. */
symbol *symtab[TABSIZE];

/* Number of symbols in symbol hash table. */
int num_hash_tab_syms = 0;

/* Count the number of nlist entries that are for local symbols.
   This count and the three following counts
   are incremented as as symbols are entered in the symbol table.  */
int local_sym_count;

/* Count number of nlist entries that are for local symbols
   whose names don't start with L. */
int non_L_local_sym_count;

/* Count the number of nlist entries for debugger info.  */
int debugger_sym_count;

/* Count the number of global symbols referenced and not defined.  */
int undefined_global_sym_count;

/* Count the number of defined global symbols.
   Each symbol is counted only once
   regardless of how many different nlist entries refer to it,
   since the output file will need only one nlist entry for it.
   This count is computed by `digest_symbols';
   it is undefined while symbols are being loaded. */
int defined_global_sym_count;

/* Count the number of indirect definitions seen on input (ie. done relative
   to the value of some other symbol. */
int global_indirect_count;

/* Total number of symbols to be written in the output file.
   Computed by digest_symbols from the variables above.  */
int nsyms;

#ifndef NO_SYMSEGS
/* Lazy evaluation of gdb symtab entries.  */
int lazy_gdb_symtab;
int used_lazy;
#endif

/* Nonzero means ptr to symbol entry for symbol to use as start addr.
   -e sets this.  */
symbol *entry_symbol;

/* Nonzero if the -A flag has been specified.  If it has then link editor
   defined symbols are not defined and no warnings are generated if they are. */
int Aflag;

/* Nonzero puts zero into mtime fields for bootstrapping compares */
int bflag = 0;

/* Each input file, and each library member ("subfile") being loaded,
   has a `file_entry' structure for it.

   For files specified by command args, these are contained in the vector
   which `file_table' points to.

   For library members, they are dynamically allocated,
   and chained through the `chain' field.
   The chain is found in the `subfiles' field of the `file_entry'.
   The `file_entry' objects for the members have `superfile' fields pointing
   to the one for the library.  */

struct file_entry {
  /* Name of this file.  */
  char *filename;
  /* Name to use for the symbol giving address of text start */
  /* Usually the same as filename, but for a file spec'd with -l
     this is the -l switch itself rather than the filename.  */
  char *local_sym_name;

  /* Describe the layout of the contents of the file */

  /* Set if this file is a mach-O file zero otherwise */
  int mach;
  /* The file's a.out header.  */
  struct exec header;
  /* The address of the file's bss section, either (header.a_text+header.a_data)
     for a.out files or the vmaddr field of a Mach-O file.  This is needed
     because of the object files created by the assembler and hacked with
     atom(1) using the -objc flag the bss address is NOT (a_text + a_data) */
  int bss_addr;
  /* Information about the sections of the objective-C segment */
  int sym_addr, mod_addr, sel_addr;
  int sym_size, mod_size, sel_size;
  int sym_reloc_size, mod_reloc_size, sel_reloc_size;
  
  /* Offsets in the file to parts of the file */
  int text_offset, data_offset, sym_offset, mod_offset, sel_offset;
  int text_reloc_offset, data_reloc_offset,
      sym_reloc_offset, mod_reloc_offset, sel_reloc_offset,
      symbols_offset, strings_offset;
#ifndef NO_SYMSEGS
  /* Offset in file of GDB symbol segment, or 0 if there is none.  */
  int symseg_offset;
#endif

  /* Describe data from the file loaded into core */

  /* Symbol table of the file.  */
  struct nlist *symbols;
  /* Size in bytes of string table.  */
  int string_size;
  /* Pointer to the string table.
     The string table is not kept in core all the time,
     but when it is in core, its address is here.  */
  char *strings;

  /* Next ones are used only if `relocatable_output' */
  struct relocation_info *textrel, *datarel, *symrel, *modrel, *selrel;

  /* Relation of this file's sections to the output file. These are the starting
     addresses of this file's sections in the output file.  They are first set
     relative to each of their sections in consider_file_section_lenghts()
     (not addresses at this point).  Then they are set to the address in the
     output file in relocate_file_addresses(). */
  int text_start_address, data_start_address, bss_start_address;
#ifdef SHLIB
  /* These two are uses in place of data_start_address in building a shlib */
  int global_data_start_address, static_data_start_address;
#endif SHLIB
  int sym_start_address, mod_start_address;

#ifdef SHLIB
  /* First global data address of this file in the input file.  The file is
     assumed to have all its statics before any globals so this address is
     lowest address of a external data symbol in the file.  This is expected
     to be the objective C factory data symbol.  All static data (which can
     vary in size) must be declared before this symbol and all global data
     (which can NOT vary in size) be declared after this symbol.  This will
     allow these objects to be placed in shared library output files by moving
     all global data of all objects first in the output followed by all static
     data.  Yes this is a very gross KLUGE. */
  int first_global_data_address;
#endif SHLIB
  /* Offset in bytes in the output file symbol table
     of the first local symbol for this file.  Set by `write_file_symbols'.  */
  int local_syms_offset;				   

  /* The selectors map of where selector strings end up in the output file */
  struct sel_map *sel_map;
  long nsels;

  /* For library members only */

  /* For a library, points to chain of entries for the library members.  */
  struct file_entry *subfiles;
  /* For a library member, offset of the member within the archive.
     Zero for files that are not library members.  */
  int starting_offset;
  /* Size of contents of this file, if library member.  */
  int total_size;
  /* For library member, points to the library's own entry.  */
  struct file_entry *superfile;
  /* For library member, points to next entry for next member.  */
  struct file_entry *chain;

  /* 1 if file is a library. */
  char library_flag;

  /* 1 if file's header has been read into this structure.  */
  char header_read_flag;

  /* 1 means search a set of directories for this file.  */
  char search_dirs_flag;

  /* 1 means this is base file of incremental load.
     Do not load this file's text or data.
     Also default text_start to after this file's bss. */
  char just_syms_flag;
};

/* Vector of entries for input files specified by arguments.
   These are all the input files except for members of specified libraries.  */
struct file_entry *file_table;

/* Length of that vector.  */
int number_of_files;

/* When loading the text and data, we can avoid doing a close
   and another open between members of the same library.

   These two variables remember the file that is currently open.
   Both are zero if no file is open.

   See `each_file' and `file_close'.  */

struct file_entry *input_file;
int input_desc;

/* The name of the file to write; "a.out" by default.  */

char *output_filename;

/* Descriptor for writing that file with `mywrite'.  */

int outdesc;

/* Header for that file (filled in by `write_header').  */

struct exec outheader;

#ifdef MACH_O
/* The type of Mach-O file to create, only valid if magic == MH_MAGIC */
long filetype = MH_EXECUTE;

/* A structure containing the things at the beginning of a Mach-O file for
   MH_EXECUTE, MH_PRELOAD and MH_FVMLIB file types. */
struct mach_aout {
	struct mach_header mh;
	struct segment_command text_seg;
	struct section text_sect;
	struct segment_command data_seg;
	struct section data_sect;
	struct section bss_sect;
	struct segment_command objc_seg;
	struct section sym_sect;
	struct section mod_sect;
	struct section sel_sect;
	struct symtab_command st;
#ifndef NO_SYMSEGS
	struct symseg_command ss;
#endif
} exec_mach;

/*
 * For MH_EXECUTE files if page zero is NOT allocated by something then a
 * segment allocating it will be created and the protections set to none.
 */
struct segment_command *pagezero_segment;

/*
 * If the -seglinkedit flag is specified then a segment containing all of the
 * link edit information (relocation entries, symbol table, string table and
 * symsegs) is created.  This is only for MH_EXECUTE files.
 */
struct segment_command *linkedit_segment;

/* A structure containing the things at the beginning of a Mach-O file for
   MH_OBJECT file type. */
struct mach_rel {
	struct mach_header mh;
	struct segment_command rel_seg;
	struct section text_sect;
	struct section data_sect;
	struct section bss_sect;
	struct section sym_sect;
	struct section mod_sect;
	struct section sel_sect;
	struct symtab_command st;
#ifndef NO_SYMSEGS
	struct symseg_command ss;
#endif
} rel_mach;
struct thread_aout {
	struct thread_command ut;
	unsigned long flavor;
	unsigned long count;
#if defined(I860)
	struct NextDimension_thread_state_regs cpu;
#else
	struct NeXT_thread_state_regs cpu;
#endif
} thread;

/* The default file type is a MH_EXECUTE so these are initialize for that.
   If the type get changed to MH_OBJECT then these will be reset. */
long mach_headers_size = sizeof(struct mach_aout);
struct mach_header *mhp = &(exec_mach.mh);
struct section *tsp = &(exec_mach.text_sect);
struct section *dsp = &(exec_mach.data_sect);
struct section *bsp = &(exec_mach.bss_sect);
struct section *symsp = &(exec_mach.sym_sect);
struct section *modsp = &(exec_mach.mod_sect);
struct section *selsp = &(exec_mach.sel_sect);
struct symtab_command *stp = &(exec_mach.st);
#ifndef NO_SYMSEGS
struct symseg_command *ssp = &(exec_mach.ss);
#endif

/* The following are used for creating mach segments from files. */
struct seg_create {
	char *begin;		/* name of the form <segment name>__begin */
	char *end;		/* name of the form <segment name>__end */
	long sizeofsects;	/* the sum of section sizes */
	char *segname;		/* full segment name from command line */
	struct segment_command sg; /* the segment load command itself */
	struct section_list *slp;  /* list of section structures */
} *seg_create;
long n_seg_create;
long seg_create_size;

struct section_list {
	char *filename;		/* file name for the contents of the section */
	long filesize;		/* the file size as returned by stat(2) */
	char *sectname;		/* full section name from command line */
	struct section s;	/* the section structure */
	char *begin;		/* name <segname><sectname>__begin */
	char *end;		/* name <segname><sectname>__end */
	struct section_list *next;	/* next section_list */
};

struct ident {
	struct ident_command lc_ident;	/* the ident load command */
	char *strings;		/* the concatinated zero terminated strings */
	long strings_size;	/* the byte count of the above strings,
				   including the zeroes and NOT rounded */
} ident;
#endif MACH_O

#ifdef MACH_SHLIB
/*
 * The following is used for mach fixed virtual address shared libraries.
 * Both for creating them and for objects that use them.
 */
struct fvmlibs {
    struct fvmlib_command lc_fvmlib; /* the load command for the mach object
				        output file */
    struct shlib_info shlib;	     /* the data read from an object file
				        for the symbol SHLIB_INFO_NAME */
    long name_size;		     /* name size (including the null) rounded
				        to sizeof(long) */
} *fvmlibs;
long n_fvmlibs;
#endif MACH_SHLIB

/*
 * The following two structures are used for creating the objective C module
 * descriptor table (pre NeXT 1.0 release).  The link editor defined symbol
 * _OBJC_BIND_SYM is the objective C 4.0 module descriptor table.  The table is
 * pairs of symbol values whos names starts with _OBJC_LINK and _OBJC_INFO and
 * have the same suffix.  The table ends with two zero values.  The table is
 * long word (32 bit) aligned in the data segment.  See ldsyms.h for the cpp
 * macro definitions of the symbol names and prefixes.
 */
struct {
    symbol *sp;			/* the _OBJC_BIND_SYM symbol */
    long data_offset;		/* offset in the data segment where it is */
    long n_bindlist;		/* the number of entries, always 1 or greater */
    struct bindlist *bindlist;	/* linked list of structures below */
} objc_bind_sym;

struct bindlist {
    symbol *link;		/* symbol with _OBJC_LINK prefix */
    symbol *info;		/* symbol with _OBJC_INFO prefix */
    struct bindlist *next;	/* next pair of symbols in the bind list */
};

/*
 * The following two structures are used for creating the objective C module
 * descriptor tables (NeXT 1.0 release).
 * The link editor defined symbol _MOD_DESC_IND_TAB is the module descriptor
 * indirect table.  This table contains pointers to module
 * descriptor tables and is terminated with a zero.  A single module descriptor
 * table is also built link editor when this symbol is referenced and is the
 * first entry in the table (but does not have a symbol).  The rest of the
 * entries in the module descriptor indirect table are the values of symbols
 * with the prefix _MOD_DESC_TAB.  The single module descriptor table that is
 * built link editor contains the values of symbols that have the prefix
 * _MOD_DESC and are not absolute symbols (N_ABS).  This table is also
 * terminated with a zero.  Both of these tables are long word (32 bit) aligned
 * and in the data segment.  The way this is intended to work is that the
 * resulting executable file will have the module descriptor indirect table
 * build by the link editor (along with the single module descriptor table) and
 * that shared libraries will have a hand built module descriptor table.
 */
struct {
    symbol *sp;			/* the _MOD_DESC_IND_TAB symbol */
    long ind_tab_offset;	/* offset in the data segment where above is */
    struct symlist *tablelist;	/* list of _MOD_DESC_TAB symbols */
    long n_tables;		/* the number of entries, always 1 or greater */
    struct symlist *desclist;	/* list of _MOD_DESC symbols */
    long n_descs;		/* the number of entries, always 1 or greater */
    long tab_offset;		/* offset in the data segment where the single
				   module descriptor is */
    long tab_value;		/* address in the data segment where the single
				   module descriptor is */
} objc_modDesc_sym;

struct symlist {
    symbol *sp;			/* the symbol */
    struct symlist *next;	/* the next symbol in the list */
};

struct {
  symbol *sp;
  long data_offset;
  long symtabsize;
  long n_globals;
} global_syminfo;

#ifdef SHLIB
/*
 * The following two structures are used for creating the link editor defined
 * symbols _SHLIB_INFO (the shared library information table) and _SHLIB_INIT
 * (the shared library initialization table).  These tables are lists of symbol
 * values whos names starts with SHLIB_SYMBOL_NAME and SHLIB_INIT_NAME
 * respectfully.  The tables both end with a zero value.  The table is long
 * word (32 bit) aligned in the data segment.  See sym.h for the cpp macro
 * definitions and the data structure of a shared library information symbol.
 */
struct {
    symbol *sp;			/* the link editor defined symbol */
    long data_offset;		/* offset in the data segment where it is */
    long n_symlist;		/* the number of symbols, always 1 or greater */
    struct symlist *symlist;	/* linked list of structures */
} shlib_info, shlib_init;

/*
 * shlib_create is set if the output file is to be a shared library. 
 */
int shlib_create;	/* output file is to be a shared library */

/*
 * Array of symbol names for -U symbol name arguments and the count of them.
 */
char **Usymnames;
int n_Usymnames;

static do_print_inits = -2;
#endif SHLIB

/*
 * This collection of variables are used in constructing the selector strings
 * section by uniqueing all of the strings in all of the selector sections of
 * all of the input files.  Also the variable sel_size is used in the
 * construction of this section and is the final size of this section in the
 * output file.
 */
/* The selector hash table; allocated if needed */
#define SELTABSIZE 1022
struct sel_bucket **seltab;

/* the hash bucket entries in the hash table points to; allocated as needed */
struct sel_bucket {
  char *sel_name;		/* name of the selector */
  long offset;			/* offset of this selector in the output file */
  struct sel_bucket *next;	/* next in the hash chain */
};

/* the blocks that store the selectors; allocated as needed */
struct sel_block {
  long size;			/* the number of bytes in sels */
  long used;			/* the number of bytes used in sels */
  long full;			/* no more sels are to be allocated in this */
  struct sel_block *next;	/* the next block */
  char *sels;			/* the selector strings */
} *sel_blocks;	/* the head of the chain of blocks */

/* the selector map structure to tell where a selector was moved to */
struct sel_map {
  long input_offset;	/* offset in the input file for the selector */
  long output_offset;	/* offset in the output file for the selector */
};

/* The following are computed by `digest_symbols'.  They are the total size of
   each of their peices in the output file. */
int text_size, data_size, bss_size;
#ifdef SHLIB
int global_data_size, static_data_size;
#endif SHLIB
int objc_size, sym_size, mod_size, sel_size;
int text_reloc_size, data_reloc_size,
    sym_reloc_size, mod_reloc_size, sel_reloc_size;

/* Amount of cleared space to leave between the text and data segments.  */

int text_pad;

/* Amount of bss segment to include as part of the data segment.  */

int data_pad;

/* Format of __.SYMDEF:
   First, a longword containing the size of the 'symdef' data that follows.
   Second, zero or more 'symdef' structures.
   Third, a word containing the length of symbol name strings.
   Fourth, zero or more symbol name strings (each followed by a zero).  */

struct symdef {
  int symbol_name_string_index;
  int library_member_offset;
};

/* Record most of the command options.  */

/* Address we assume the text segment will be loaded at.
   We relocate symbols and text and data for this, but we do not
   write any padding in the output file for it.  */
int text_start;

/* Offset of default entry-pc within the text section.  */
int entry_offset;

/* Address we decide the data segment will be loaded at.  */
int data_start;
#ifdef SHLIB
/* Addresses of the global and static data will be loaded at if creating a shlib
  (based on data_start).*/
int global_data_start, static_data_start;
#endif SHLIB
/* Address of other the objective-C segment (follows data segment) */
int objc_start;

/* `text-start' address is normally this much plus a page boundary.
   This is not a user option; it is fixed for each system.  */
int text_start_alignment;

/* Nonzero if -T was specified in the command line.
   This prevents text_start from being set later to default values.  */
int T_flag_specified;

/* Nonzero if -Tdata was specified in the command line.
   This prevents data_start from being set later to default values.  */
int Tdata_flag_specified;

/* Size to pad data section up to.
   We simply increase the size of the data section, padding with zeros,
   and reduce the size of the bss section to match.  */
int specified_data_size;

/* Magic number to use for the output file, set by switch.  */
int magic = MH_MAGIC;

/* Nonzero means print names of input files as processed.  */
int trace_files;

/* Which symbols should be stripped (omitted from the output):
   none, all, or debugger symbols.  */
enum { STRIP_NONE, STRIP_ALL, STRIP_DEBUGGER } strip_symbols = STRIP_NONE;

/* Which local symbols should be omitted:
   none, all, or those starting with L.
   This is irrelevant if STRIP_NONE.  */
enum { DISCARD_NONE, DISCARD_ALL, DISCARD_L } discard_locals = DISCARD_NONE;

/* 1 => write load map.  */
int write_map;

/* 1 => write relocation into output file so can re-input it later.  */
int relocatable_output;

/* 1 => assign space to common symbols even if `relocatable_output'.  */
int force_common_definition;

/* -Z inhibits the default search_dirs for cross compilation paranoia. */
int inhibit_default_search_dirs = 0;

/* Standard directories to search for files specified by -l.  */
char *standard_search_dirs[] = {
	"/usr/next_cross/lib", "/usr/next_cross/usr/lib", 
	"/lib", "/usr/lib", "/usr/local/lib"};

/* Actual vector of directories to search;
   this contains those specified with -L plus the standard ones.  */
char **search_dirs;

/* Length of the vector `search_dirs'.  */
int n_search_dirs;

/* Nonzero means should make the output file executable when done.  */
/* Cleared by nonfatal errors.  */
int make_executable = 1;

/*
 * Gets set by calls to fatal_error_with_file() or fatal_error() so
 * all such errors can be processed before exiting.
 */
int fatal_errors = 0;

int verbose;

void digest_symbols ();
void print_symbols ();
void load_symbols ();
void ld_defined_symbols();
void set_ld_defined_sym();
void decode_command ();
void list_undefined_symbols ();
void write_output ();
void write_header ();
void write_text ();
void write_data ();
void write_objc ();
void write_seg_create ();
void write_rel ();
void write_syms ();
void padfile ();
char *concat ();
symbol *getsym (), *getsym_soft ();
#ifdef SYM_ALIAS
void set_alias_values (), remove_aliased_symbols ();
symbol *removed_aliased_symbols;
#endif SYM_ALIAS
long round();

main (argc, argv)
     char **argv;
     int argc;
{
  page_size = PAGESIZE;

  /* Completely decode ARGV.  */

  decode_command (argc, argv);

  /* Load symbols of all input files.
     Also search all libraries and decide which library members to load.  */

  load_symbols ();

  /* Determine whether to count the header as part of
     the text size, and initialize the text size accordingly.
     This depends on the kind of system and on the output format selected.  */

  if (!T_flag_specified) {
#ifdef MACH_O
      if (magic == MH_MAGIC)
	{
	  if (filetype == MH_EXECUTE)
	    text_start = PAGSIZ;
	  else
	    text_start = 0;
	}
      else
#endif MACH_O
	{
	  text_start = N_TXTADDR (outheader);
	}
#ifdef SHLIB
      if (shlib_create)
	fatal ("-T must be specified for shared library output");
#endif SHLIB
    }

  /* The text-start address is normally this far past a page boundary.  */
  text_start_alignment = text_start % page_size;

#ifdef MACH_O
  if (magic == MH_MAGIC)
    {
      if (shlib_create)
	  mach_headers_size = PAGSIZ;
      else if (filetype == MH_EXECUTE || filetype == MH_PRELOAD)
	  mach_headers_size += sizeof(struct thread_aout);
	
#ifdef PAGEZERO_PROTECT
      if (filetype == MH_EXECUTE && text_start != 0 &&
	  (data_start != 0 || !Tdata_flag_specified))
	{
	  pagezero_segment = (struct segment_command *)
			     xmalloc (sizeof(struct segment_command));
	  mach_headers_size += sizeof(struct segment_command);
	}
#endif PAGEZERO_PROTECT
#if defined(I860)
      /*
       * At this point, we know how big the mach header will be.
       * We adjust the size of the header so that the following text
       * segment will start on a 32 byte boundry, satisfying the ill-documented
       * alignment requirements of the i860.
       *
       * The header size is adjusted by creating or growing a LC_IDENT
       * command in the header, probably the most harmless thing we could
       * do.
       */
      if( filetype != MH_OBJECT && (mach_headers_size & 0x1F) != 0)
      {
      	  int pad = 32 - (mach_headers_size & 0x1F);
	  int i;
	  
	  if (ident.strings_size == 0)
	  { /* re-adjust pad allowing for new header. */
	    pad = 32 - ((mach_headers_size + sizeof(struct ident_command))&0x1F);
	    if ( pad == 0 )
	    	pad = 32;	/* Round to next boundry, for sanity */
	    ident.strings = (char *) xmalloc(pad);
	    ident.strings_size = pad;
	    for( i = 0; i < pad; ++i )
	    	ident.strings[i] = '\0';
	    ident.lc_ident.cmd = LC_IDENT;
	    ident.lc_ident.cmdsize = sizeof(struct ident_command)+ident.strings_size;
	    mach_headers_size += ident.lc_ident.cmdsize;
	  }
	  else
	  {
	    ident.strings = (char *) xrealloc(ident.strings,
				     ident.strings_size + pad);
	    for ( i = ident.strings_size; i < (ident.strings_size + pad); ++i )
		ident.strings[i] = '\0';
	    ident.strings_size += pad;
	    ident.lc_ident.cmdsize = sizeof(struct ident_command)+ident.strings_size;
	    mach_headers_size += pad;
	  }
      }
#endif

      if (mach_headers_size > PAGSIZ)
	fatal ("mach headers size (0x%x) exceeds pagesize (0x%x)",
	       mach_headers_size, PAGSIZ);

      if (filetype == MH_EXECUTE || filetype == MH_FVMLIB)
	text_size = mach_headers_size;
      else
	text_size = 0;
    }
  else
#endif MACH_O
    {
      text_size = sizeof (struct exec) - N_TXTOFF (outheader);
      if (text_size < 0)
	text_size = 0;
    }
  entry_offset = text_size;


  /* Compute where each file's sections go, and relocate symbols.  */

  digest_symbols ();

  /* Defined loader defined symbols if they are referenced */

  ld_defined_symbols ();

#ifdef SYM_ALIAS
  set_alias_values ();
#endif SYM_ALIAS

  /* Print error messages for any missing symbols.  */
  list_undefined_symbols (stderr);

  /* Print a map, if requested.  */

  if (write_map) print_symbols (stdout);

  /* Write the output file.  */
  if (make_executable || relocatable_output)
    write_output ();

  if (make_executable == 0 && !relocatable_output)
    exit (1);
  else{
    exit (0);
  }
}

void decode_option ();

/* Analyze a command line argument.
   Return 0 if the argument is a filename.
   Return 1 if the argument is a option complete in itself.
   Return 2 if the argument is a option which uses an argument.
   Return 3 if the argument is a option which uses two arguments.
   Return 4 if the argument is a option which uses three arguments.

   Thus, the value is the number of consecutive arguments
   that are part of options.  */

int
classify_arg (arg)
     register char *arg;
{
  if (*arg != '-') return 0;
  switch (arg[1])
    {
    case 'A':
    case 'D':
    case 'e':
    case 'L':
    case 'l':
    case 'o':
    case 'u':
    case 'U':
    case 'y':
#ifdef SYM_ALIAS
    case 'a':
#endif SYM_ALIAS
      if (arg[2])
	return 1;
      return 2;
    case 'T':
      if (arg[2] == 0)
	return 2;
      if (! strcmp (&arg[2], "text"))
	return 2;
      if (! strcmp (&arg[2], "data"))
	return 2;
      return 1;
    case 's':
#ifdef MACH_O
      if (strcmp(arg, "-segcreate") == 0)
	return 4;
#endif MACH_O
      return 1;
#ifdef MACH_O
    case 'i':
      if (strcmp(arg, "-ident") == 0)
	return 2;
#endif MACH_O
    }

  return 1;
}

/* Process the command arguments,
   setting up file_table with an entry for each input file,
   and setting variables according to the options.  */

void
decode_command (argc, argv)
     char **argv;
     int argc;
{
  register int i, j, k;
  register struct file_entry *p;

  number_of_files = 0;
  output_filename = "a.out";

  n_search_dirs = 0;
  search_dirs = (char **) xmalloc (sizeof (char *));

  /* First compute number_of_files so we know how long to make file_table.  */
  /* Also process most options completely.  */

  for (i = 1; i < argc; i++)
    {
      register int code = classify_arg (argv[i]);
      if (code)
	{
	  if (i + code > argc)
	    if (code > 2)
	      fatal ("not enough arguments following %s", argv[i]);
	    else
	      fatal ("no argument following %s", argv[i]);

	  if (code == 4)
	    decode_option (argv[i], argv[i+1], argv[i+2], argv[i+3]);
	  else if (code == 3)
	    decode_option (argv[i], argv[i+1], argv[i+2], "");
	  else
	    decode_option (argv[i], argv[i+1], "", "");

#ifdef APPKIT_HACK
	  /* SCS: This must go!  Temporary hack for appkit */
	  if (!strcmp(argv[i], "-lappkit") ||
	      !strcmp(argv[i], "-lappkit_g") ||
	      !strcmp(argv[i], "-lappkit_p"))
	    number_of_files += 4;
#endif APPKIT_HACK
	  if (argv[i][1] == 'l' || argv[i][1] == 'A')
	    number_of_files++;

	  i += code - 1;
	}
      else
	number_of_files++;
    }

  if (!number_of_files)
    fatal ("no input files");

  p = file_table
    = (struct file_entry *) xmalloc (number_of_files * sizeof (struct file_entry));
  bzero (p, number_of_files * sizeof (struct file_entry));

  /* Now scan again and fill in file_table.  */
  /* All options except -A and -l are ignored here.  */

  for (i = 1; i < argc; i++)
    {
      register int code = classify_arg (argv[i]);

      if (code)
	{
	  char *string;
	  if (code == 2)
	    string = argv[i+1];
	  else
	    string = &argv[i][2];

	  if (argv[i][1] == 'A')
	    {
	      if (p != file_table)
		fatal ("-A specified before an input file other than the first");

	      Aflag = 1;
	      p->filename = string;
	      p->local_sym_name = string;
	      p->just_syms_flag = 1;
	      p++;
	    }
	  if (argv[i][1] == 'l')
	    {
	      char *cp = rindex(string, '.');
	      /* If the filename ends in .o, don't prepend "lib" to it */
	      if (cp && !strcmp(cp, ".o"))
		p->filename = string;
	      else
	        p->filename = concat ("lib", string, ".a");
	      p->local_sym_name = concat ("-l", string, "");
	      p->search_dirs_flag = 1;
	      p++;

#ifdef APPKIT_HACK
	/* More -lappkit hackery */
#define ADDLIB(s) p->filename = concat ("lib", s, ".a"); \
      p->local_sym_name = concat ("-l", string, ""); \
      p->search_dirs_flag = 1; p++;

	      if (!strcmp(string, "appkit")) {
		ADDLIB("psconnect");
		ADDLIB("utilities");
		ADDLIB("objc");
		ADDLIB("m");
	      } else if (!strcmp(string, "appkit_g")) {
		ADDLIB("psconnect_g");
		ADDLIB("utilities_g");
		ADDLIB("objc_g");
		ADDLIB("m");
	      } else if (!strcmp(string, "appkit_p")) {
		ADDLIB("psconnect_p");
		ADDLIB("utilities_p");
		ADDLIB("objc_p");
		ADDLIB("m");
	      }
#undef ADDLIB
#endif APPKIT_HACK
	    }
	  i += code - 1;
	}
      else
	{
	  p->filename = argv[i];
	  p->local_sym_name = argv[i];
	  p++;
	}
    }

#ifdef MACH_O
  /*
   * Now for each segment created from files round the vmsize to the pagesize
   * and set the filesize.  Also sum up the total size into seg_create_size.
   */
  for (i = 0; i < n_seg_create; i++)
    {
      if ((magic == MH_MAGIC && filetype ==  MH_EXECUTE) || shlib_create)
        seg_create[i].sg.vmsize = (seg_create[i].sizeofsects + page_size - 1) &
				  (- page_size);
      else
        seg_create[i].sg.vmsize = seg_create[i].sizeofsects;
      seg_create[i].sg.filesize = seg_create[i].sg.vmsize;
      seg_create_size += seg_create[i].sg.vmsize;
    }

  if (ident.strings_size != 0){
    i = ident.strings_size;
    ident.strings_size = round(ident.strings_size, sizeof(long));
    ident.strings = (char *) xrealloc(ident.strings, ident.strings_size);
    for (j = i; j < ident.strings_size; j++)
      ident.strings[j] = '\0';
    ident.lc_ident.cmd = LC_IDENT;
    ident.lc_ident.cmdsize = sizeof(struct ident_command) +
			     ident.strings_size;
    mach_headers_size += ident.lc_ident.cmdsize;
  }

#endif MACH_O

  /* Now check some option settings for consistency.  */

  if ((magic == ZMAGIC || magic == NMAGIC ||
      (magic == MH_MAGIC && filetype == MH_EXECUTE) || shlib_create)
      && (text_start - text_start_alignment) & (page_size - 1))
    fatal ("-T argument not multiple of page size (0x%x), with sharable output",
	   page_size);

  /* Append the standard search directories to the user-specified ones.  */
  if (!inhibit_default_search_dirs)
  {
    int n = sizeof standard_search_dirs / sizeof standard_search_dirs[0];
    n_search_dirs += n;
    search_dirs
      = (char **) xrealloc (search_dirs, n_search_dirs * sizeof (char *));
    bcopy (standard_search_dirs, &search_dirs[n_search_dirs - n],
	   n * sizeof (char *));
  }

  for (i = 0; i < n_seg_create; i++)
    {
      struct section_list *slp1, *slp2;

      for (j = i + 1; j < n_seg_create; j++)
	{
	  if (strncmp(seg_create[i].segname, seg_create[j].segname,
		      sizeof(seg_create->sg.segname)) == 0)
	    fatal_error ("segment names: %s and %s not unique to %d "
			 "characters\n", seg_create[i].segname,
			 seg_create[j].segname, sizeof(seg_create->sg.segname));

	}

      slp1 = seg_create[i].slp;
      for (j = 0; j < seg_create[i].sg.nsects; j++)
	{
	  slp2 = slp1->next;
	  for (k = j + 1; k < seg_create[i].sg.nsects; k++)
	    {
	      if (strncmp(slp1->sectname, slp2->sectname,
			  sizeof(slp1->s.sectname)) == 0)
		fatal_error ("section names: %s and %s (in segment %s) not "
			     "unique to %d characters\n", slp1->sectname,
			     slp2->sectname, seg_create[i].segname,
			     sizeof(slp1->s.sectname));
	      slp2 = slp2->next;
	    }
	  slp1 = slp1->next;
	}
    }
  if (fatal_errors)
    exit (1);
}

/* Record an option and arrange to act on it later.
   ARG should be the following command argument,
   which may or may not be used by this option.

   The `l' and `A' options are ignored here since they actually
   specify input files.  */

void
decode_option (swt, arg, arg2, arg3)
     register char *swt, *arg, *arg2, *arg3;
{
  if (! strcmp (swt, "-Ttext"))
    {
      text_start = parse (arg, "%x", "invalid argument to -Ttext");
      T_flag_specified = 1;
      return;
    }
  if (! strcmp (swt, "-Tdata"))
    {
      data_start = parse (arg, "%x", "invalid argument to -Tdata");
      Tdata_flag_specified = 1;
      return;
    }
#ifdef MACH_O
  /* This has to be here because -xFOO is treated the same way as -x FOO
     and this precludes multicharacter option names unless done here.
	arg  - is the segment name
	arg2 - is the section name
	arg3 - is the file name  */
  if (strcmp(swt, "-segcreate") == 0)
    {
      struct stat statbuf;
      struct segment_command *sgp;
      struct section_list *slp;
      long i, j, seglen, sectlen;
      char *p;

      if (magic == MH_MAGIC && filetype == MH_OBJECT)
	{
	  error ("-segcreate ignored with relocatable output");
	  return;
	}
      if (strcmp(SEG_TEXT, arg) == 0 || strcmp(SEG_DATA, arg) == 0 ||
          strcmp(SEG_OBJC, arg) == 0)
	fatal ("can't used reserved segment name: %s with -segcreate", arg);
      if (stat(arg3, &statbuf) == -1)
	fatal ("can't stat() file: %s (to create a segment with)", arg3);
      if (n_seg_create == 0)
	{
	  seg_create = (struct seg_create *)
			xmalloc (sizeof (struct seg_create));
	  seg_create[0].slp = (struct section_list *)
			      xmalloc (sizeof (struct section_list));
	}
      else {
	/*
	 * First see if this segment already exists.
	 */
	for (i = 0; i < n_seg_create; i++)
	  {
	    sgp = &(seg_create[i].sg);
	    if (strcmp(seg_create[i].segname, arg) == 0)
	      {
		slp = seg_create[i].slp;
		for (j = 0; j < sgp->nsects; j++)
		  {
		    if (strcmp(slp->sectname, arg2) == 0)
		       fatal ("more that one -segcreate option with the same "
			      "segment (%s) and section (%s) name", arg, arg2);
		    slp = slp->next;
		  }
		break;
	      }
	  }
	/*
	 * If this is just another section in an already existing segment then
	 * just create another section list structure for it.
	 */
	if (i != n_seg_create)
	  {
#if defined(I860)
	    int align = 2;	/* 2^2 default alignment */
	    
	    if ( strcmp(arg2,"__text") == 0 && strcmp(arg3, "__TEXT") == 0 )
	    	align = 5;	/* 256 bit alignment for insns! */
	    else if ( strcmp(arg3, "__DATA") == 0 )
	    	align = 4;	/* 128 bit alignment for data. */
#endif
	    slp = seg_create[i].slp;
	    for (j = 0; j < sgp->nsects-1 ; j++)
	      slp = slp->next;
	    slp->next = (struct section_list *)
			xmalloc(sizeof(struct section_list));
	    slp = slp->next;
	    bzero((char *)slp, sizeof(struct section_list));
	    slp->sectname = arg2;
	    slp->filename = arg3;
	    slp->filesize = statbuf.st_size;
	    strncpy(slp->s.sectname, arg2, sizeof(slp->s.sectname));
	    strncpy(slp->s.segname, arg, sizeof(slp->s.segname));
	    slp->s.addr = 0; /* filled in later */
	    slp->s.size = statbuf.st_size;
	    slp->s.size = round(slp->s.size, sizeof(long));
	    slp->s.offset = 0; /* filled in later */
#if defined(I860)
	    slp->s.align = align;
#else
	    slp->s.align = 2;
#endif
	    /* all other fields zero */

	    /* create strings for the begin and end loader defined symbols */
	    seglen = strlen(arg);
	    sectlen = strlen(arg2);
	    p = (char *) xmalloc (seglen + sectlen + sizeof("__begin") + 1);
	    bzero(p, seglen + sectlen + sizeof("__begin") + 1);
	    strncpy(p, arg, sizeof(slp->s.segname));
	    strncat(p, arg2, sizeof(slp->s.sectname));
	    strcat(p, "__begin");
	    slp->begin = p;
	    p = (char *) xmalloc (seglen + sectlen + sizeof("__end") + 1);
	    bzero(p, seglen + sectlen + sizeof("__end") + 1);
	    strncpy(p, arg, sizeof(slp->s.segname));
	    strncat(p, arg2, sizeof(slp->s.sectname));
	    strcat(p, "__end");
	    slp->end = p;

	    mach_headers_size += sizeof(struct section);
	    seg_create[i].sizeofsects += slp->s.size;
	    sgp->cmdsize += sizeof(struct section);
	    sgp->nsects++;
	    return;
	  }
	/*
	 * This is a new segment, create the structures to hold the info
	 * for it.
	 */
	seg_create = (struct seg_create *)xrealloc(seg_create,
			(n_seg_create + 1) * sizeof (struct seg_create));
	seg_create[n_seg_create].slp = (struct section_list *)
			    xmalloc (sizeof (struct section_list));
      }

      slp = seg_create[n_seg_create].slp;
      bzero((char *)slp, sizeof(struct section_list));
      slp->sectname = arg2;
      slp->filename = arg3;
      slp->filesize = statbuf.st_size;
      sectlen = strlen(arg2);
      strncpy(slp->s.sectname, arg2, sizeof(slp->s.sectname));
      seglen = strlen(arg);
      strncpy(slp->s.segname, arg, sizeof(slp->s.segname));
      slp->s.addr = 0; /* filled in later */
      slp->s.size = statbuf.st_size;
      slp->s.size = round(slp->s.size, sizeof(long));
#if defined(I860)
      if ( strcmp(arg2,"__text") == 0 && strcmp(arg3, "__TEXT") == 0 )
	slp->s.align = 5;	/* 256 bit alignment for insns! */
      else if ( strcmp(arg3, "__DATA") == 0 )
	slp->s.align = 4;	/* 128 bit alignment for data. */
      else
	slp->s.align = 2;	/* long aligned *.
#else
      slp->s.align = 2; /* long aligned (2^2) */
#endif
      slp->s.offset = 0; /* filled in later */
      /* all other fields zero */

      /* create strings for the begin and end loader defined symbols */
      p = (char *) xmalloc (seglen + sectlen + sizeof("__begin") + 1);
      bzero(p, seglen + sectlen + sizeof("__begin") + 1);
      strncpy(p, arg, sizeof(slp->s.segname));
      strncat(p, arg2, sizeof(slp->s.sectname));
      strcat(p, "__begin");
      slp->begin = p;
      p = (char *) xmalloc (seglen + sectlen + sizeof("__end") + 1);
      bzero(p, seglen + sectlen + sizeof("__end") + 1);
      strncpy(p, arg, sizeof(slp->s.segname));
      strncat(p, arg2, sizeof(slp->s.sectname));
      strcat(p, "__end");
      slp->end = p;

      seg_create[n_seg_create].segname = arg;
      seg_create[n_seg_create].sizeofsects = slp->s.size;

      sgp = &(seg_create[n_seg_create].sg);
      bzero((char *)sgp, sizeof(struct segment_command));
      sgp->cmd = LC_SEGMENT;
      sgp->cmdsize = sizeof(struct segment_command) + sizeof(struct section);
      strncpy(sgp->segname, slp->s.segname, sizeof(sgp->segname));
      sgp->vmaddr = 0; /* filled in later */
      sgp->vmsize = 0; /* filled in later */;
      sgp->fileoff = 0; /* filled in later */
      sgp->filesize = 0; /* set to vmsize later */
      sgp->maxprot = VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXECUTE;
      sgp->initprot =VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXECUTE;
      sgp->nsects = 1;
      sgp->flags = 0;


      /* create strings for the begin and end loader defined symbols */
      p = (char *) xmalloc (seglen + sizeof("__begin") + 1);
      bzero(p, seglen + sizeof("__begin") + 1);
      strncpy(p, arg, sizeof(slp->s.segname));
      strcat(p, "__begin");
      seg_create[n_seg_create].begin = p;
      p = (char *) xmalloc (seglen + sizeof("__end") + 1);
      bzero(p, seglen + sizeof("__end") + 1);
      strncpy(p, arg, sizeof(slp->s.segname));
      strcat(p, "__end");
      seg_create[n_seg_create].end = p;

      n_seg_create++;
      mach_headers_size += sgp->cmdsize;
      return;
    }
  if (strcmp(swt, "-ident") == 0)
    {
      if (magic == MH_MAGIC && filetype == MH_OBJECT)
	{
	  error ("-ident ignored with relocatable output");
	  return;
	}
      if (ident.strings_size == 0)
	ident.strings = (char *) xmalloc(strlen(arg) + 1);
      else
	ident.strings = (char *) xrealloc(ident.strings,
				 ident.strings_size + strlen(arg) + 1);
      strcat(ident.strings + ident.strings_size, arg);
      ident.strings_size += strlen(arg) + 1;
      return;
    }
  if (strcmp(swt, "-seglinkedit") == 0)
    {
      linkedit_segment = (struct segment_command *)
			 xmalloc (sizeof(struct segment_command));
      mach_headers_size += sizeof(struct segment_command);
      return;
    }
#endif MACH_O

  if (swt[2] != 0)
    arg = &swt[2];

  switch (swt[1])
    {
    case 'A':
      return;
#ifdef SYM_ALIAS
    case 'a':
    case 'i':	/* For forward compatibility! */
      {
	char *p;
	register symbol *sp;

	  p = index(arg, ':');
	  if (p == (char *)0 || p[1] == '\0')
	    fatal ("-a argument: %s must have a ':' between it's names", arg);
	  *p = '\0';
	  p++;
	  sp = getsym (p);
	  if (!sp->defined && !sp->referenced)
	    undefined_global_sym_count++;
	  sp->referenced = 1;
	  sp = getsym (arg);
	  if (!sp->defined && !sp->referenced)
	    undefined_global_sym_count++;
	  sp->referenced = 1;
	  sp->alias_name = p;
	  return;
      }
#endif SYM_ALIAS
    case 'b':
      bflag = 1;
      return;

#ifdef AOUT_SHLIB
    case 'c':
      magic = LIBMAGIC;
      shlib_create = 1;
      return;
#endif AOUT_SHLIB

    case 'D':
      specified_data_size = parse (arg, "%x", "invalid argument to -D");
      return;

    case 'd':
      force_common_definition = 1;
      return;

    case 'e':
      if (strcmp(swt, "-emacs") == 0)
        {
	  magic = ZMAGIC;
	  outheader.a_magic = magic;
	  return;
        }
      entry_symbol = getsym (arg);
      if (!entry_symbol->defined && !entry_symbol->referenced)
	undefined_global_sym_count++;
      entry_symbol->referenced = 1;
      return;

#ifdef MACH_SHLIB
    case 'f':
      shlib_create = 1;
      magic = MH_MAGIC;
      if (filetype == MH_OBJECT)
        mach_headers_size += sizeof(struct mach_aout) - sizeof(struct mach_rel);
      filetype = MH_FVMLIB;
      mhp = &(exec_mach.mh);
      tsp = &(exec_mach.text_sect);
      dsp = &(exec_mach.data_sect);
      bsp = &(exec_mach.bss_sect);
      symsp = &(exec_mach.sym_sect);
      modsp = &(exec_mach.mod_sect);
      selsp = &(exec_mach.sel_sect);
      stp = &(exec_mach.st);
#ifndef NO_SYMSEGS
      ssp = &(exec_mach.ss);
#endif
      return;
#endif MACH_SHLIB

#ifndef NO_SYMSEGS
    case 'g':
      lazy_gdb_symtab = 1;
      return;

    case 'G':
      lazy_gdb_symtab = 0;
      return;
#endif

#ifdef SHLIB
    case 'I':
      do_print_inits = 0;
      return;
#endif SHLIBS

    case 'K':
      mach_syms = 0;
      return;

    case 'l':
      return;

    case 'L':
      n_search_dirs++;
      search_dirs
	= (char **) xrealloc (search_dirs, n_search_dirs * sizeof (char *));
      search_dirs[n_search_dirs - 1] = arg;
      return;

    case 'M':
#ifdef MACH_O
      if (strcmp(swt, "-Mach") == 0)
	{
	  magic = MH_MAGIC;
	  if (filetype == MH_OBJECT)
	    mach_headers_size += sizeof(struct mach_aout) -
				 sizeof(struct mach_rel);
	  filetype = MH_EXECUTE;
	  mhp = &(exec_mach.mh);
	  tsp = &(exec_mach.text_sect);
	  dsp = &(exec_mach.data_sect);
	  bsp = &(exec_mach.bss_sect);
	  symsp = &(exec_mach.sym_sect);
	  modsp = &(exec_mach.mod_sect);
	  selsp = &(exec_mach.sel_sect);
	  stp = &(exec_mach.st);
#ifndef NO_SYMSEGS
	  ssp = &(exec_mach.ss);
#endif
	  return;
	}
#endif MACH_O
      
      write_map = 1;
      return;

#ifdef 0
    case 'N':
      magic = OMAGIC;
      outheader.a_magic = magic;
      return;

    case 'n':
      magic = NMAGIC;
      outheader.a_magic = magic;
      return;
#endif 0

    case 'o':
      output_filename = arg;
      return;

    case 'p':
      magic = MH_MAGIC;
      if (filetype == MH_OBJECT)
        mach_headers_size += sizeof(struct mach_aout) - sizeof(struct mach_rel);
      filetype = MH_PRELOAD;
      mhp = &(exec_mach.mh);
      tsp = &(exec_mach.text_sect);
      dsp = &(exec_mach.data_sect);
      bsp = &(exec_mach.bss_sect);
      symsp = &(exec_mach.sym_sect);
      modsp = &(exec_mach.mod_sect);
      selsp = &(exec_mach.sel_sect);
      stp = &(exec_mach.st);
#ifndef NO_SYMSEGS
      ssp = &(exec_mach.ss);
#endif
      return;

#ifdef 0
    case 'R':
      relocatable_output = 1;
#ifndef NO_SYMSEGS
      lazy_gdb_symtab = 0;
#endif
      magic = OMAGIC;
      outheader.a_magic = magic;
      return;
#endif 0

    case 'r':
      relocatable_output = 1;
#ifndef NO_SYMSEGS
      lazy_gdb_symtab = 0;
#endif
      magic = MH_MAGIC;
      if (filetype != MH_OBJECT)
	mach_headers_size += sizeof(struct mach_rel) -
			     sizeof(struct mach_aout);
      filetype = MH_OBJECT;
      mhp = &(rel_mach.mh);
      tsp = &(rel_mach.text_sect);
      dsp = &(rel_mach.data_sect);
      bsp = &(rel_mach.bss_sect);
      symsp = &(rel_mach.sym_sect);
      modsp = &(rel_mach.mod_sect);
      selsp = &(rel_mach.sel_sect);
      stp = &(rel_mach.st);
#ifndef NO_SYMSEGS
      ssp = &(rel_mach.ss);
#endif
      return;

    case 'S':
      strip_symbols = STRIP_DEBUGGER;
      return;

    case 's':
      strip_symbols = STRIP_ALL;
      return;

    case 'T':
      text_start = parse (arg, "%x", "invalid argument to -T");
      T_flag_specified = 1;
      return;

    case 't':
      trace_files = 1;
      return;

    case 'U':
      if(n_Usymnames == 0)
	Usymnames = (char **)xmalloc(sizeof(char *));
      else
	Usymnames = (char **)xrealloc(Usymnames,
				      (n_Usymnames + 1) * sizeof(char *));
      Usymnames[n_Usymnames++] = arg;
      return;

    case 'u':
      {
	register symbol *sp = getsym (arg);
	if (!sp->defined && !sp->referenced) undefined_global_sym_count++;
	sp->referenced = 1;
      }
      return;

    case 'v':
      verbose = 1;
      return;

    case 'X':
      discard_locals = DISCARD_L;
      return;

    case 'x':
      discard_locals = DISCARD_ALL;
      return;

    case 'y':
      {
	register symbol *sp = getsym (&swt[2]);
	sp->trace = 1;
      }
      return;

#ifdef 0
    case 'z':
      magic = ZMAGIC;
      outheader.a_magic = magic;
      return;
#endif 0

    case 'Z':
      inhibit_default_search_dirs = 1;
      return;

    default:
      fatal ("invalid command option `%s'", swt);
    }
}

/** Convenient functions for operating on one or all files being loaded.  */

/* Call FUNCTION on each input file entry.
   Do not call for entries for libraries;
   instead, call once for each library member that is being loaded.

   FUNCTION receives two arguments: the entry, and ARG.  */

void
each_file (function, arg)
     register void (*function)();
     register int arg;
{
  register int i;

  for (i = 0; i < number_of_files; i++)
    {
      register struct file_entry *entry = &file_table[i];
      if (entry->library_flag)
        {
	  register struct file_entry *subentry = entry->subfiles;
	  for (; subentry; subentry = subentry->chain)
	    (*function) (subentry, arg);
	}
      else
	(*function) (entry, arg);
    }
}

/* Like `each_file' but ignore files that were just for symbol definitions.  */

void
each_full_file (function, arg)
     register void (*function)();
     register int arg;
{
  register int i;

  for (i = 0; i < number_of_files; i++)
    {
      register struct file_entry *entry = &file_table[i];
      if (entry->just_syms_flag)
	continue;
      if (entry->library_flag)
        {
	  register struct file_entry *subentry = entry->subfiles;
	  for (; subentry; subentry = subentry->chain)
	    (*function) (subentry, arg);
	}
      else
	(*function) (entry, arg);
    }
}

/* Close the input file that is now open.  */

void
file_close ()
{
  if (input_desc >= 0)
    close (input_desc);
  input_desc = 0;
  input_file = 0;
}

/* Open the input file specified by 'entry', and return a descriptor.
   The open file is remembered; if the same file is opened twice in a row,
   a new open is not actually done.  */

int
file_open (entry)
     register struct file_entry *entry;
{
  register int desc;

  if (entry->superfile)
    return file_open (entry->superfile);

  if (entry == input_file)
    return input_desc;

  if (input_file) file_close ();

  if (entry->search_dirs_flag)
    {
      register char **p = search_dirs;
      int i;

      for (i = 0; i < n_search_dirs; i++)
	{
	  register char *string
	    = concat (search_dirs[i], "/", entry->filename);
	  desc = open (string, O_RDONLY, 0);
	  if (desc > 0)
	    {
	      entry->filename = string;
	      entry->search_dirs_flag = 0;
	      break;
	    }
	  free (string);
	}
    }
  else
    desc = open (entry->filename, O_RDONLY, 0);

  if (desc >= 0)
    {
      input_file = entry;
      input_desc = desc;
      return desc;
    }

  perror_fatal_with_file (entry, "Can't open file: ");
}

/* Print the filename of ENTRY on OUTFILE (a stdio stream),
   and then a newline.  */

prline_file_name (entry, outfile)
     struct file_entry *entry;
     FILE *outfile;
{
  print_file_name (entry, outfile);
  fprintf (outfile, "\n");
}

/* Print the filename of ENTRY on OUTFILE (a stdio stream).  */

print_file_name (entry, outfile)
     struct file_entry *entry;
     FILE *outfile;
{
  if (entry->superfile)
    {
      print_file_name (entry->superfile, outfile);
      fprintf (outfile, "(%s)", entry->filename);
    }
  else
    fprintf (outfile, "%s", entry->filename);
}

/* Medium-level input routines for rel files.  */

/* Read a file's header into the proper place in the file_entry.
   DESC is the descriptor on which the file is open.
   ENTRY is the file's entry.  */

void
read_header (desc, entry)
     int desc;
     register struct file_entry *entry;
{
  register int len;
  struct exec *loc = &entry->header;

  lseek (desc, entry->starting_offset, 0);
  len = read (desc, loc, sizeof (struct exec));
  if (len != sizeof (struct exec))
    fatal_with_file (entry, "Can't read header of: ");
#ifdef 0
  if (!N_BADMAG (*loc))
    {
      entry->text_offset = N_TXTOFF(entry->header);
      entry->data_offset = N_TXTOFF(entry->header) + entry->header.a_text;
      entry->text_reloc_offset = N_TXTOFF(entry->header) +
				 entry->header.a_text + entry->header.a_data;
      entry->data_reloc_offset = N_TXTOFF(entry->header) +
				 entry->header.a_text + entry->header.a_data +
				 entry->header.a_trsize;
      entry->symbols_offset = N_SYMOFF(entry->header);
      entry->strings_offset = N_STROFF(entry->header);
      entry->bss_addr = N_BSSADDR(entry->header);
    }
  else
#endif 0
  if (*((long *)loc) == MH_MAGIC)
    {
      int i, j;
      struct mach_header mh;
      struct load_command *load_commands, *lcp;
      struct segment_command *sgp;
      struct section *sp, *text_sect, *data_sect, *bss_sect,
      		     *sym_sect, *mod_sect, *sel_sect;
      struct symtab_command *stp;
#ifndef NO_SYMSEGS
      struct symseg_command *ssp;
#endif

      /* zero since it has been read into and is trash */
      bzero(&entry->header, sizeof(struct exec));

      lseek (desc, entry->starting_offset, 0);
      len = read (desc, (char *)&mh, sizeof (struct mach_header));
      if (len != sizeof (struct mach_header))
	fatal_with_file (entry, "Can't read header of: ");
      if(entry->just_syms_flag)
	{
	  if(mh.filetype != MH_EXECUTE && mh.filetype != MH_PRELOAD)
	    fatal_with_file (entry, "Bad file type number (not MH_EXECUTE or "
			     "MH_PRELOAD) in: ");
	}
      else
	{
	  if(mh.filetype != MH_OBJECT)
	    fatal_with_file(entry, "Bad file type number (not MH_OBJECT) in: ");
#if defined(I860)
	  if(mh.cputype != MACHINE_CPU_TYPE)
	    fatal_with_file(entry, "Bad CPU type number (not i860) in: ");
	  if(mh.cpusubtype != MACHINE_CPU_SUBTYPE)
	    fatal_with_file(entry, "Bad CPU subtype number (not NextDimension) in: ");
#endif
	}

      load_commands = (struct load_command *)xmalloc(mh.sizeofcmds);
      len = read (desc, (char *)load_commands, mh.sizeofcmds);
      if (len != mh.sizeofcmds)
	fatal_with_file (entry, "Can't read load commands of: ");

      sgp = NULL;
      text_sect = NULL;
      data_sect = NULL;
      bss_sect = NULL;
      sym_sect = NULL;
      mod_sect = NULL;
      sel_sect = NULL;
      stp = NULL;
#ifndef NO_SYMSEGS
      ssp = NULL;
#endif
      lcp = load_commands;
      for (i = 0; i < mh.ncmds; i++)
	{
	  if(lcp->cmdsize % sizeof(long) != 0)
	      fatal_with_file (entry, "Load command %d size not a multiple of "
			       "sizeof(long) in: ", i);
	  if(lcp->cmdsize <= 0)
	      fatal_with_file (entry, "Load command %d size is less than or "
			       "equal to zero in: ", i);
	  if((char *)lcp + lcp->cmdsize > (char *)load_commands + mh.sizeofcmds)
	      fatal_with_file (entry, "Load command %d extends past end of all "
			       "load commands in: ", i);
	  switch(lcp->cmd)
	    {
	      case LC_SEGMENT:
		sgp = (struct segment_command *)lcp;
		if(sgp->cmdsize != sizeof(struct segment_command) +
				     sgp->nsects * sizeof(struct section))
		  fatal_with_file (entry, "Load command %d inconsistant size "
				   "for segment command in: ", i);
		if(sgp->flags & SG_FVMLIB || entry->just_syms_flag)
		  break;
		if(sgp->vmaddr != 0)
		  fatal_with_file (entry, "relocatable object not loaded at "
				   "zero: ");
		sp = (struct section *)
			((char *)sgp + sizeof(struct segment_command));
		for (j = 0; j < sgp->nsects; j++)
		  {
		    if(strcmp(sp->sectname, SECT_TEXT) == 0)
		      {
			if(text_sect != NULL)
			  fatal_with_file (entry, "More than one %s section "
					   "in: ", SECT_TEXT);
			text_sect = sp;
		      }
		    else if(strcmp(sp->sectname, SECT_DATA) == 0)
		      {
			if(data_sect != NULL)
			  fatal_with_file (entry, "More than one %s section "
					   "in: ", SECT_DATA);
			data_sect = sp;
		      }
		    else if(strcmp(sp->sectname, SECT_BSS) == 0)
		      {
			if(bss_sect != NULL)
			  fatal_with_file (entry, "More than one %s section "
					   "in: ", SECT_BSS);
			bss_sect = sp;
		      }
		    else if(strcmp(sp->sectname, SECT_OBJC_SYMBOLS) == 0)
		      {
			if(sym_sect != NULL)
			  fatal_with_file (entry, "More than one %s section "
					   "in: ", SECT_OBJC_SYMBOLS);
			sym_sect = sp;
		      }
		    else if(strcmp(sp->sectname, SECT_OBJC_MODULES) == 0)
		      {
			if(mod_sect != NULL)
			  fatal_with_file (entry, "More than one %s section "
					   "in: ", SECT_OBJC_MODULES);
			mod_sect = sp;
		      }
		    else if(strcmp(sp->sectname, SECT_OBJC_STRINGS) == 0)
		      {
			if(sel_sect != NULL)
			  fatal_with_file (entry, "More than one %s section "
					   "in: ", SECT_OBJC_STRINGS);
			sel_sect = sp;
		      }
		    else
		      {
			  fatal_with_file (entry, "Unknown section %s in: ",
					   sp->sectname);
		      }
		    sp++;
		  }
		break;
	      case LC_SYMTAB:
		if(lcp->cmdsize < sizeof(struct symtab_command))
		  fatal_with_file (entry, "Load command %d size too small "
				   "for symtab command in: ", i);
		if(stp == NULL)
		  stp = (struct symtab_command *)lcp;
		break;
	      case LC_SYMSEG:
#ifndef NO_SYMSEGS
		if(lcp->cmdsize < sizeof(struct symseg_command))
		  fatal_with_file (entry, "Load command %d size too small "
				   "for symseg command in: ", i);
		if(ssp == NULL)
		  ssp = (struct symseg_command *)lcp;
		break;
#endif
	      case LC_THREAD:
	      case LC_UNIXTHREAD:
	      case LC_LOADFVMLIB:
	      case LC_IDFVMLIB:
	      case LC_IDENT:
		break;
	      default:
	        error_with_file (entry, "Load command %d of unknown type", i);
		break;
	    }
	  lcp = (struct load_command *)((char *)lcp + lcp->cmdsize);
	}

      entry->mach = 1;
      if (text_sect != NULL)
	{
	  entry->text_offset = text_sect->offset;
	  entry->text_reloc_offset = text_sect->reloff;
	  entry->header.a_text = text_sect->size;
	  entry->header.a_trsize =
		text_sect->nreloc * sizeof(struct relocation_info);
	}
      if (data_sect != NULL)
	{
	  entry->data_offset = data_sect->offset;
	  entry->data_reloc_offset = data_sect->reloff;
	  entry->header.a_data = data_sect->size;
	  entry->header.a_drsize =
		data_sect->nreloc * sizeof(struct relocation_info);
	}
      if (bss_sect != NULL)
	{
	  entry->header.a_bss = bss_sect->size;
	  entry->bss_addr = bss_sect->addr;
	}
      if (sym_sect != NULL)
	{
	  entry->sym_addr = sym_sect->addr;
	  entry->sym_size = sym_sect->size;
	  entry->sym_offset = sym_sect->offset;
	  entry->sym_reloc_offset = sym_sect->reloff;
	  entry->sym_reloc_size = sym_sect->nreloc *
				  sizeof(struct relocation_info);
	}
      if (mod_sect != NULL)
	{
	  entry->mod_addr = mod_sect->addr;
	  entry->mod_size = mod_sect->size;
	  entry->mod_offset = mod_sect->offset;
	  entry->mod_reloc_offset = mod_sect->reloff;
	  entry->mod_reloc_size = mod_sect->nreloc *
				  sizeof(struct relocation_info);
	}
      if (sel_sect != NULL)
	{
	  entry->sel_addr = sel_sect->addr;
	  entry->sel_size = sel_sect->size;
	  entry->sel_offset = sel_sect->offset;
	  entry->sel_reloc_offset = sel_sect->reloff;
	  entry->sel_reloc_size = sel_sect->nreloc *
				  sizeof(struct relocation_info);
	}
      if (stp != NULL)
	{
	  entry->symbols_offset = stp->symoff;
	  entry->strings_offset = stp->stroff;
	  entry->header.a_syms = stp->nsyms * sizeof(struct nlist);
	  entry->string_size = stp->strsize;
	}
#ifndef NO_SYMSEGS
      if (ssp != NULL && ssp->size != 0)
	entry->symseg_offset = ssp->offset;
#endif
    }
  else
    fatal_with_file (entry, "Bad magic number in: ");

  entry->header_read_flag = 1;
}

/* Read the symbols of file ENTRY into core.
   Assume it is already open, on descriptor DESC.
   Also read the length of the string table, which follows the symbol table,
   but don't read the contents of the string table.  */

void
read_entry_symbols (desc, entry)
     struct file_entry *entry;
     int desc;
{
  int str_size;

  if (!entry->header_read_flag)
    read_header (desc, entry);

  entry->symbols = (struct nlist *) xmalloc (entry->header.a_syms);

  lseek (desc, entry->starting_offset + entry->symbols_offset, 0);
  if (entry->header.a_syms != read (desc, entry->symbols, entry->header.a_syms))
    fatal_with_file (entry, "Premature end of file in symbols of: ");

  if (!entry->mach)
    {
      lseek (desc, entry->starting_offset + entry->strings_offset, 0);
      if (sizeof str_size != read (desc, &str_size, sizeof str_size))
	fatal_with_file (entry, "Bad string table size in: ");

      entry->string_size = str_size;
    }
}

/* Read the string table of file ENTRY into core.
   Assume it is already open, on descriptor DESC.
   Also record whether a GDB symbol segment follows the string table.  */

void
read_entry_strings (desc, entry)
     struct file_entry *entry;
     int desc;
{
  int buffer;

  if (!entry->header_read_flag)
    read_header (desc, entry);

  lseek (desc, entry->starting_offset + entry->strings_offset, 0);
  if (entry->string_size != read (desc, entry->strings, entry->string_size))
    fatal_with_file (entry, "Premature end of file in strings of: ");

  if (!entry->mach)
    {
      /* While we are here, see if the file has a symbol segment at the end.
	 For a separate file, just try reading some more.
	 For a library member, compare current pos against total size.  */
      if (entry->superfile)
	{
	  if (entry->total_size == entry->strings_offset + entry->string_size)
	    return;
	}
      else
	{
	  buffer = read (desc, &buffer, sizeof buffer);
	  if (buffer == 0)
	    return;
	  if (buffer != sizeof buffer)
	    fatal_with_file (entry, "Premature end of file in GDB symbol "
			     "segment of: ");
	}

#ifndef NO_SYMSEGS
      entry->symseg_offset = entry->strings_offset + entry->string_size;
#endif
    }
}

/* Read in the symbols of all input files.  */

void read_file_symbols (), read_entry_symbols (), read_entry_strings ();
void map_file_selectors ();
void enter_file_symbols (), enter_global_ref (), search_library ();

void
load_symbols ()
{
  register int i;

  if (trace_files) fprintf (stderr, "Loading symbols:\n\n");

  for (i = 0; i < number_of_files; i++)
    {
      register struct file_entry *entry = &file_table[i];
      read_file_symbols (entry);
    }

  if (trace_files) fprintf (stderr, "\n");

#ifdef SHLIBS
  if (fatal_errors)
    exit (1);
#endif SHLIBS
}

/* If ENTRY is a rel file, read its symbol and string sections into core.
   If it is a library, search it and load the appropriate members
   (which means calling this function recursively on those members).  */

void
read_file_symbols (entry)
     register struct file_entry *entry;
{
  register int desc;
  register int len;
  int magicnum;

  desc = file_open (entry);

  len = read (desc, &magicnum, sizeof magicnum);
  if (len != sizeof magicnum)
    fatal_with_file (entry, "Failure reading header of: ");

  if (!N_BADMAG (*((struct exec *)&magicnum)) ||
      *((long *)&magicnum) == MH_MAGIC )
    {
      read_entry_symbols (desc, entry);
      entry->strings = (char *) xmalloc (entry->string_size);
      read_entry_strings (desc, entry);
      enter_file_symbols (desc, entry);
      free (entry->strings);
      entry->strings = 0;
      map_file_selectors (desc, entry);
    }
  else
    {
      char armag[SARMAG];

      lseek (desc, 0, 0);
      if (SARMAG != read (desc, armag, SARMAG) || strncmp (armag, ARMAG, SARMAG))
	fatal_with_file (entry, "Malformed input file (not rel or archive): ");
      entry->library_flag = 1;
      search_library (desc, entry);
    }

  file_close ();
}

/* Enter the external symbol defs and refs of ENTRY in the hash table.  */
#ifdef SHLIB
/* For shared library output determine the first global data address
   for this file.  The global data must be after all the static data */
#endif SHLIB

void
enter_file_symbols (desc, entry)
     int desc;
     struct file_entry *entry;
{
   register struct nlist *p, *end = entry->symbols + entry->header.a_syms / sizeof (struct nlist);
#ifdef SHLIB
   long last_static_data_address;
#endif SHLIB

  if (trace_files) prline_file_name (entry, stderr);

#ifdef SHLIB
  if (shlib_create)
    {
      entry->first_global_data_address = entry->header.a_text +
					 entry->header.a_data;
      last_static_data_address = entry->header.a_text;
    }
#endif SHLIB

  for (p = entry->symbols; p < end; p++)
    {
      if (p->n_un.n_strx < 0 || p->n_un.n_strx > entry->string_size)
	fatal_with_file (entry, "Bad string table index (%d) for symbol "
			 "(%d) in: ", p->n_un.n_strx, (p - entry->symbols) /
			 sizeof(struct nlist) );
      if (p->n_type & N_EXT)
	{
	  enter_global_ref (p, p->n_un.n_strx + entry->strings, entry, desc);
#ifdef SHLIB
	  if (shlib_create && ((p->n_type & N_TYPE) == N_DATA ||
	      ((p->n_type & N_TYPE) == N_SECT && p->n_sect == 2) ) &&
	      p->n_value < entry->first_global_data_address)
	    entry->first_global_data_address = p->n_value;
#endif SHLIB
	}
      else if (p->n_un.n_strx && !(p->n_type & (N_STAB | N_EXT)))
	{
	  if ((p->n_un.n_strx + entry->strings)[0] != 'L')
	    {
	      non_L_local_sym_count++;
#ifdef SHLIB
	      if (shlib_create && ((p->n_type & N_TYPE) == N_DATA ||
		  ((p->n_type & N_TYPE) == N_SECT && p->n_sect == 2) ) &&
		  p->n_value > last_static_data_address)
		last_static_data_address = p->n_value;
#endif SHLIB
	    }
	  local_sym_count++;
	}
      else debugger_sym_count++;
    }

   /* Count one for the local symbol that we generate,
      whose name is the file's name (usually) and whose address
      is the start of the file's text.  */

  local_sym_count++;
  non_L_local_sym_count++;

#ifdef SHLIB
/*
printf("For %s first_global_data_address = 0x%x\n",
entry->filename, entry->first_global_data_address);
*/
  if (shlib_create &&
      last_static_data_address > entry->first_global_data_address)
    fatal_error_with_file (entry, "not all global data appears after static "
			   "data in: ");
#endif SHLIB
}

/* Enter one global symbol in the hash table.
   NLIST_P points to the `struct nlist' read from the file
   that describes the global symbol.  NAME is the symbol's name.
   ENTRY is the file entry for the file the symbol comes from.

   The `struct nlist' is modified by placing it on a chain of
   all such structs that refer to the same global symbol.
   This chain starts in the `refs' field of the symbol table entry
   and is chained through the `n_name'.  */

void
enter_global_ref (nlist_p, name, entry, desc)
     register struct nlist *nlist_p;
     char *name;
     struct file_entry *entry;
     int desc;
{
  register symbol *sp = getsym (name);
  register int type = nlist_p->n_type;
  int oldref = sp->referenced;
  int olddef = sp->defined;

  nlist_p->n_un.n_name = (char *) sp->refs;
  sp->refs = nlist_p;

  sp->referenced = 1;

  if (type != (N_UNDF | N_EXT) || nlist_p->n_value)
    {
      sp->defined = 1;
      if (oldref && !olddef) undefined_global_sym_count--;
      /* If this is a common definition, keep track of largest
	 common definition seen for this symbol.  */
      if (type == (N_UNDF | N_EXT)
	  && sp->max_common_size < nlist_p->n_value) {
#ifndef NO_SYMSEGS
	/* SCS: if lazysyms, record the filename so gdb can 
	   know which object file has correct type info for
	   this symbol */
	if (lazy_gdb_symtab)
	  sp->common_file_entry = entry;
#endif
	sp->max_common_size = nlist_p->n_value;
      }
      else if (type == (N_INDR | N_EXT))
	{
#ifdef DEBUG
printf("N_INDR %s refering to %s\n", name,  nlist_p->n_value + entry->strings);
#endif DEBUG
	  nlist_p->n_value =
		(unsigned int) getsym (nlist_p->n_value + entry->strings);
	  global_indirect_count++;
	}
#ifdef MACH_SHLIB
      else{
	/*
	 * See if this symbol is a shared library information symbol.  If so
	 * read the contents of that symbol.
	 */
	if (!Aflag && (!relocatable_output || force_common_definition) &&
	    strncmp(name, SHLIB_SYMBOL_NAME, sizeof(SHLIB_SYMBOL_NAME)-1) == 0)
	  {
	    if (type == (N_TEXT | N_EXT) ||
		(type == (N_SECT | N_EXT) && nlist_p->n_sect == 1) ||
		type == (N_DATA | N_EXT) ||
		(type == (N_SECT | N_EXT) && nlist_p->n_sect == 2) )
	      {
		if (type == (N_TEXT | N_EXT) ||
		    (type == (N_SECT | N_EXT) && nlist_p->n_sect == 1) )
		  {
		    if (nlist_p->n_value + sizeof(struct shlib_info) >
			entry->header.a_text)
		      fatal ("shared library information symbol: %s too "
			     "small", name);
		    lseek (desc, entry->starting_offset +
			   entry->text_offset + nlist_p->n_value, L_SET);
		  }
		else
		  {
		    if ((nlist_p->n_value - entry->header.a_text) +
			sizeof(struct shlib_info) > entry->header.a_data)
		      fatal ("shared library information symbol: %s too "
			     "small", name);
		    lseek (desc, entry->starting_offset + entry->data_offset
			   + (nlist_p->n_value - entry->header.a_text), L_SET);
		  }
		if( n_fvmlibs == 0)
		  fvmlibs = (struct fvmlibs *)xmalloc(sizeof (struct fvmlibs));
		else
		  fvmlibs = (struct fvmlibs *)xrealloc(fvmlibs,
				(n_fvmlibs + 1) * sizeof (struct fvmlibs));
		if (read(desc, &(fvmlibs[n_fvmlibs].shlib),
		    sizeof(struct shlib_info)) != sizeof(struct shlib_info))
		  fatal_with_file (entry, "Can't read shared library "
				   "information symbol: %s from: ", name);
/*
printf("shared library information symbol: %s target name : %s\n", name, fvmlibs[n_fvmlibs].shlib.name);
*/
		if (shlib_create)
		  fvmlibs[n_fvmlibs].lc_fvmlib.cmd = LC_IDFVMLIB;
		else
		  fvmlibs[n_fvmlibs].lc_fvmlib.cmd = LC_LOADFVMLIB;
		fvmlibs[n_fvmlibs].name_size =
			strlen(fvmlibs[n_fvmlibs].shlib.name) + 1;
		fvmlibs[n_fvmlibs].name_size =
			round(fvmlibs[n_fvmlibs].name_size, sizeof(long));
		fvmlibs[n_fvmlibs].lc_fvmlib.cmdsize =
			sizeof(struct fvmlib_command) +
			fvmlibs[n_fvmlibs].name_size;
		fvmlibs[n_fvmlibs].lc_fvmlib.fvmlib.name.offset =
			sizeof(struct fvmlib_command);
		fvmlibs[n_fvmlibs].lc_fvmlib.fvmlib.minor_version = 
			fvmlibs[n_fvmlibs].shlib.minor_version;
		fvmlibs[n_fvmlibs].lc_fvmlib.fvmlib.header_addr = 
			fvmlibs[n_fvmlibs].shlib.text_addr;
		mach_headers_size += fvmlibs[n_fvmlibs].lc_fvmlib.cmdsize;
		n_fvmlibs++;
	      }
	    else
	      error ("shared library information symbol: %s not a text or data"
		     "symbol", name);
	  }
      }
#endif MACH_SHLIB
    }
  else
    if (!oldref) undefined_global_sym_count++;

  if ( entry->just_syms_flag && !T_flag_specified && 
       strcmp(sp->name, _END) == 0)
    {
      text_start = nlist_p->n_value;
      if (magic == MH_MAGIC && filetype == MH_EXECUTE)
	text_start = round(text_start, page_size);
      T_flag_specified = 1;
    }

  if (sp->trace)
    {
      register char *reftype;
      reftype = "unknown definition or reference";
      switch (type & N_TYPE)
	{
	case N_UNDF:
	  if (nlist_p->n_value)
	    reftype = "defined as common";
	  else reftype = "referenced";
	  break;

	case N_ABS:
	  reftype = "defined as absolute";
	  break;

	case N_TEXT:
	  reftype = "defined in text section";
	  break;

	case N_DATA:
	  reftype = "defined in data section";
	  break;

	case N_BSS:
	  reftype = "defined in bss section";
	  break;

	case N_SECT:
	  switch (nlist_p->n_sect)
	    {
	    case 1:
	      reftype = "defined in text section";
	      break;
	    case 2:
	      reftype = "defined in data section";
	      break;
	    case 3:
	      reftype = "defined in bss section";
	      break;
	    case 4:
	      reftype = "defined in objective-C symbol section";
	      break;
	    case 5:
	      reftype = "defined in objective-C module section";
	      break;
	    case 6:
	      reftype = "defined in objective-C string section";
	      break;
	    }
	  break;
	case N_INDR:
	  reftype = (char *) alloca (23 +
			strlen (((symbol *)nlist_p->n_value)->name));
	  sprintf (reftype, "defined equivalent to %s",
		   ((symbol *)nlist_p->n_value)->name);
	  break;
	}

      fprintf (stderr, "symbol %s %s in ", sp->name, reftype);
      print_file_name (entry, stderr);
      fprintf (stderr, "\n");
    }
}

/* Create the selector map for this file.  Read the selector section of file
   ENTRY into core.  Assume it is already open, on descriptor DESC.  */

void
map_file_selectors (desc, entry)
     int desc;
     struct file_entry *entry;
{
  char *selectors, *p;
  long i, offset;

  if (entry->sel_size == 0)
    return;

  selectors = (char *) xmalloc (entry->sel_size + 1);

  offset = lseek (desc, 0, L_INCR);
  lseek (desc, entry->starting_offset + entry->sel_offset, L_SET);
  if (entry->sel_size != read (desc, selectors, entry->sel_size))
    fatal_with_file (entry, "Premature end of file in selectors of: ");
  lseek (desc, offset, L_SET);

  selectors[entry->sel_size] = '\0';

  entry->nsels = 0;
  for(p = selectors; p < selectors + entry->sel_size; p += strlen(p) + 1)
    entry->nsels++;

  entry->sel_map = (struct sel_map *) xmalloc (entry->nsels *
						sizeof(struct sel_map));
  p = selectors;
/*
printf("In map_file_selectors for %s (0x%x) nsels = %d\n", entry->filename, p, entry->nsels);
*/
  for (i = 0; i < entry->nsels; i++)
    {
      entry->sel_map[i].input_offset = p - selectors;
      entry->sel_map[i].output_offset = getsel(p);
/*
printf("\t%d, %d (%s)\n", entry->sel_map[i].input_offset, entry->sel_map[i].output_offset, p);
*/
      p += strlen(p) + 1;
    }

 free (selectors);
}

/* Searching libraries */

struct file_entry *decode_library_subfile ();
void linear_library (), symdef_library ();

/* These are used to check the date of the archive vs the __.SYMDEF file. */
struct ar_hdr hdr1;
struct stat statbuf;

/* Search the library ENTRY, already open on descriptor DESC.
   This means deciding which library members to load,
   making a chain of `struct file_entry' for those members,
   and entering their global symbols in the hash table.  */

void
search_library (desc, entry)
     int desc;
     struct file_entry *entry;
{
  int member_length;
  register char *name;
  register struct file_entry *subentry;

  if (!undefined_global_sym_count) return;

  /* Examine its first member, which starts SARMAG bytes in.  */
  subentry = decode_library_subfile (desc, entry, SARMAG, &member_length);
  if (!subentry) return;

  name = subentry->filename;

  /* Search via __.SYMDEF if that exists, else linearly.  */

  if (!strcmp (name, "__.SYMDEF")){
    if (fstat(desc, &statbuf) == -1)
      fatal_with_file (entry, "Can't stat: ");
    if (statbuf.st_mtime > atol(hdr1.ar_date))
      {
        error("warning: table of contents for archive: %s is out of date; "
	      "rerun ranlib(1)", entry->filename);
	linear_library (desc, entry, SARMAG + sizeof(struct ar_hdr) +
				     member_length);
      }
    else
      symdef_library (desc, entry, member_length);
  }
  else{
    error("warning: archive: %s has no table of contents; add one using "
	  "ranlib(1)", entry->filename);
    linear_library (desc, entry, 0);
  }

  free (subentry);
}

/* Construct and return a file_entry for a library member.
   The library's file_entry is library_entry, and the library is open on DESC.
   SUBFILE_OFFSET is the byte index in the library of this member's header.
   We store the length of the member into *LENGTH_LOC.  */

struct file_entry *
decode_library_subfile (desc, library_entry, subfile_offset, length_loc)
     int desc;
     struct file_entry *library_entry;
     int subfile_offset;
     int *length_loc;
{
  int bytes_read;
  register int namelen;
  int member_length;
  register char *name;
  register struct file_entry *subentry;

  lseek (desc, subfile_offset, 0);

  bytes_read = read (desc, &hdr1, sizeof hdr1);
  if (!bytes_read)
    return 0;		/* end of archive */

  if (sizeof hdr1 != bytes_read)
    fatal_with_file (library_entry, "Malformed library archive: ");

  if (sscanf (hdr1.ar_size, "%d", &member_length) != 1)
    fatal_with_file (library_entry,
		     "Malformatted header of archive member in: ");

  subentry = (struct file_entry *) xmalloc (sizeof (struct file_entry));
  bzero (subentry, sizeof (struct file_entry));

  for (namelen = 0;
       namelen < sizeof hdr1.ar_name
       && hdr1.ar_name[namelen] != 0 && hdr1.ar_name[namelen] != ' ';
       namelen++);

  name = (char *) xmalloc (namelen+1);
  strncpy (name, hdr1.ar_name, namelen);
  name[namelen] = 0;

  subentry->filename = name;
  subentry->local_sym_name = name;
  subentry->symbols = 0;
  subentry->strings = 0;
  subentry->subfiles = 0;
  subentry->starting_offset = subfile_offset + sizeof hdr1;
  subentry->superfile = library_entry;
  subentry->library_flag = 0;
  subentry->header_read_flag = 0;
  subentry->just_syms_flag = 0;
  subentry->chain = 0;
  subentry->total_size = member_length;

  (*length_loc) = member_length;

  return subentry;
}

/* Search a library that has a __.SYMDEF member.
   DESC is a descriptor on which the library is open.
     The file pointer is assumed to point at the __.SYMDEF data.
   ENTRY is the library's file_entry.
   MEMBER_LENGTH is the length of the __.SYMDEF data.  */

void
symdef_library (desc, entry, member_length)
     int desc;
     struct file_entry *entry;
     int member_length;
{
  int *symdef_data = (int *) xmalloc (member_length);
  register struct symdef *symdef_base;
  char *sym_name_base;
  int number_of_symdefs;
  int length_of_strings;
  int not_finished;
  int bytes_read;
  register int i;
  struct file_entry *prev = 0;
  int prev_offset = 0;

  bytes_read = read (desc, symdef_data, member_length);
  if (bytes_read != member_length)
    fatal_with_file (entry, "Malformatted __.SYMDEF in: ");

  number_of_symdefs = *symdef_data / sizeof (struct symdef);
  if (number_of_symdefs < 0 ||
       number_of_symdefs * sizeof (struct symdef) + 2 * sizeof (int) >= member_length)
    fatal_with_file (entry, "Malformatted __.SYMDEF in: ");

  symdef_base = (struct symdef *) (symdef_data + 1);
  length_of_strings = *(int *) (symdef_base + number_of_symdefs);

  if (length_of_strings < 0
      || number_of_symdefs * sizeof (struct symdef) + length_of_strings
	  + 2 * sizeof (int) != member_length)
    fatal_with_file (entry, "Malformatted __.SYMDEF in: ");

  sym_name_base = sizeof (int) + (char *) (symdef_base + number_of_symdefs);

  /* Check all the string indexes and offsets for validity.  */
  if (fstat(desc, &statbuf) == -1)
    fatal_with_file (entry, "Can't stat: ");
  for (i = 0; i < number_of_symdefs; i++)
    {
      register int index = symdef_base[i].symbol_name_string_index;
      register int offset = symdef_base[i].library_member_offset;
      if (index < 0 || index >= length_of_strings
	  || (index && *(sym_name_base + index - 1)))
	fatal_with_file (entry, "Malformatted __.SYMDEF in: ");
      if (offset < 0 || offset >= statbuf.st_size)
	fatal_with_file (entry, "Malformatted __.SYMDEF in: ");
    }

  /* Search the symdef data for members to load.
     Do this until one whole pass finds nothing to load.  */

  not_finished = 1;
  while (not_finished)
    {
      not_finished = 0;

      /* Scan all the symbols mentioned in the symdef for ones that we need.
	 Load the library members that contain such symbols.  */

      for (i = 0; i < number_of_symdefs && undefined_global_sym_count; i++)
	if (symdef_base[i].symbol_name_string_index >= 0)
	  {
	    register symbol *sp;

	    sp = getsym_soft (sym_name_base
			      + symdef_base[i].symbol_name_string_index);

	    /* If we find a symbol that appears to be needed, think carefully
	       about the archive member that the symbol is in.  */

	    if (sp && sp->referenced && !sp->defined)
	      {
		int junk;
		register int j;
		register int offset = symdef_base[i].library_member_offset;
		struct file_entry *subentry;

		/* Don't think carefully about any archive member
		   more than once in a given pass.  */

		if (prev_offset == offset)
		  continue;
		prev_offset = offset;

		/* Read the symbol table of the archive member.  */

		subentry = decode_library_subfile (desc, entry, offset, &junk);
		read_entry_symbols (desc, subentry);
		subentry->strings = (char *) xmalloc (subentry->string_size);
		read_entry_strings (desc, subentry);

		/* Now scan the symbol table and decide whether to load.  */

		if (!subfile_wanted_p (subentry))
		  {
		    free (subentry->symbols);
		    free (subentry);
		  }
		else
		  {
		    /* This member is needed; load it.
		       Since we are loading something on this pass,
		       we must make another pass through the symdef data.  */

		    not_finished = 1;

		    enter_file_symbols (desc, subentry);
		    map_file_selectors (desc, subentry);

		    if (prev)
		      prev->chain = subentry;
		    else entry->subfiles = subentry;
		    prev = subentry;

		    /* Clear out this member's symbols from the symdef data
		       so that following passes won't waste time on them.  */

		    for (j = 0; j < number_of_symdefs; j++)
		      {
			if (symdef_base[j].library_member_offset == offset)
			  symdef_base[j].symbol_name_string_index = -1;
		      }
		  }

		/* We'll read the strings again if we need them again.  */
		free (subentry->strings);
	      }
	  }
    }

  free (symdef_data);
}

/* Search a library that has no __.SYMDEF.
   ENTRY is the library's file_entry.
   DESC is the descriptor it is open on.  */

void
linear_library (desc, entry, this_subfile_offset)
     int desc;
     struct file_entry *entry;
     register int this_subfile_offset;
{
  register struct file_entry *prev = 0;

  while (undefined_global_sym_count)
    {
      int member_length;
      register struct file_entry *subentry;

      subentry = decode_library_subfile (desc, entry, this_subfile_offset, &member_length);

      if (!subentry) return;

      read_entry_symbols (desc, subentry);
      subentry->strings = (char *) xmalloc (subentry->string_size);
      read_entry_strings (desc, subentry);

      if (!subfile_wanted_p (subentry))
	{
	  free (subentry->symbols);
	  free (subentry->strings);
	  free (subentry);
	}
      else
	{
	  enter_file_symbols (desc, subentry);
	  map_file_selectors (desc, subentry);

	  if (prev)
	    prev->chain = subentry;
	  else entry->subfiles = subentry;
	  prev = subentry;

	  free (subentry->strings);
	  subentry->strings = NULL;
	}

      this_subfile_offset += member_length + sizeof (struct ar_hdr);
      if (this_subfile_offset & 1)
	this_subfile_offset++;

    }
}

/* ENTRY is an entry for a library member.
   Its symbols have been read into core, but not entered.
   Return nonzero if we ought to load this member.  */

int
subfile_wanted_p (entry)
     struct file_entry *entry;
{
  register struct nlist *p;
  register struct nlist *end
    = entry->symbols + entry->header.a_syms / sizeof (struct nlist);

  for (p = entry->symbols; p < end; p++)
    {
      register int type = p->n_type;

      if (p->n_un.n_strx < 0 || p->n_un.n_strx > entry->string_size)
	fatal_with_file (entry, "Bad string table index (%d) for symbol "
			 "(%d) in: ", p->n_un.n_strx, (p - entry->symbols) /
			 sizeof(struct nlist) );

      if (type & N_EXT && (type != (N_UNDF | N_EXT) || p->n_value))
	{
	  register char *name = p->n_un.n_strx + entry->strings;
	  register symbol *sp = getsym_soft (name);

	  /* If this symbol has not been hashed, we can't be looking for it. */

	  if (!sp) continue;

	  if (sp->referenced && !sp->defined)
	    {
	      /* This is a symbol we are looking for.  */
	      if (type == (N_UNDF | N_EXT))
		{
		  /* Symbol being defined as common.
		     Remember this, but don't load subfile just for this.  */

		  if (sp->max_common_size < p->n_value)
		    sp->max_common_size = p->n_value;
		  if (!sp->defined)
		    undefined_global_sym_count--;
		  sp->defined = 1;
		  continue;
		}

	      if (write_map)
		{
		  print_file_name (entry, stdout);
		  fprintf (stdout, " needed due to %s\n", sp->name);
		}
	      return 1;
	    }
	}
    }

  return 0;
}

void consider_file_section_lengths (), relocate_file_addresses ();
void objc_bind (), objc_modDesc (), gen_global_symtab_info(), shlib_syms();

/* Having entered all the global symbols and found the sizes of sections
   of all files to be linked, make all appropriate deductions from this data.

   We propagate global symbol values from definitions to references.
   We compute the layout of the output file and where each input file's
   contents fit into it.  */


void
digest_symbols ()
{
  register int i, j;

  if (trace_files)
    fprintf (stderr, "Digesting symbol information:\n\n");

  /* Compute total size of sections */

  each_file (consider_file_section_lengths, 0);

  /* Build all the link editor defined symbols if they are used.  They
     are allocated in the data section and thus effects it's size. */
#ifdef SHLIB
  if ((!relocatable_output || force_common_definition) && !shlib_create) {
#else
  if (!relocatable_output || force_common_definition) {
#endif SHLIB
    /* objective C module description tables */
    objc_bind ();	
    objc_modDesc ();
  }

#ifdef SHLIB
  if (!relocatable_output || force_common_definition)
    shlib_syms ();		/* the shared library tables */
#endif SHLIB

#ifdef SHLIB
  if ((!relocatable_output || force_common_definition) && !shlib_create) {
#else
  if (!relocatable_output || force_common_definition) {
#endif SHLIB
    /* This must come AFTER all global symbols have been defined */
    gen_global_symtab_info();	/* global symbol table for dynaload */
  }

  if (magic == MH_MAGIC)
   {
      if (filetype == MH_EXECUTE || filetype == MH_FVMLIB)
	tsp->size = text_size - mach_headers_size;
      else
	tsp->size = text_size;
   }

  /* If necessary, pad text section to full page in the file.
     Include the padding in the text segment size.  */
#if defined(I860)
  if (magic == NMAGIC || magic == ZMAGIC ||
      (magic == MH_MAGIC && (filetype ==  MH_EXECUTE || filetype ==  MH_PRELOAD)) ||
       shlib_create)
#else
  if (magic == NMAGIC || magic == ZMAGIC ||
      (magic == MH_MAGIC && filetype ==  MH_EXECUTE) || shlib_create)
#endif
    {
      int text_end;
#ifdef MACH_O
      if (magic == MH_MAGIC)
	  text_end = text_size;
      else
#endif MACH_O
	text_end = text_size + N_TXTOFF (outheader);
      text_pad = ((text_end + page_size - 1) & (- page_size)) - text_end;
      text_size += text_pad;
    }

  outheader.a_text = text_size;

  /* Make the data segment address start in memory on a suitable boundary.  */

  if (! Tdata_flag_specified)
    {
#ifdef SHLIB
      if (shlib_create)
	fatal ("-Tdata must be specified for shared library output");
#endif SHLIB
#ifdef MACH_O
      if (magic == MH_MAGIC)
	data_start = text_start + text_size;
      else
#endif MACH_O
	data_start = N_DATADDR (outheader) + text_start - N_TXTADDR (outheader);
    }
#ifdef SHLIB
  if (shlib_create)
    {
      global_data_start = data_start;
      static_data_start = data_start + global_data_size;
    }
#endif SHLIB

  /* Now, allocate common symbols in the BSS section so the final bss_size
     is known and the objc_start can be set. */
  bss_size = round (bss_size, sizeof(long));
  for (i = 0; i < TABSIZE; i++)
    {
      register symbol *sp;
      for (sp = symtab[i]; sp; sp = sp->link)
	{
	  register struct nlist *p, *next;
	  int defs = 0, com = sp->max_common_size, erred = 0;
	  for (p = sp->refs; p; p = next)
	    {
	      register int type = p->n_type;
	      if ((type & N_EXT) && type != (N_UNDF | N_EXT))
		{
		  /* non-common definition */
		  defs++;
		}
	      next = (struct nlist *) p->n_un.n_name;
	    }
	  /* Allocate as common if defined as common and not defined for real */
	  if (com && !defs)
	    {
	      if (!relocatable_output || force_common_definition)
		{
#ifdef SHLIB
		  if (shlib_create)
		    fatal_error ("common symbol: %s not allowed with shared "
				 "library output", sp->name);
#endif SHLIB
		  com = (com + sizeof (int) - 1) & (- sizeof (int));
    
		  sp->value = data_start + data_size + bss_size;
		  if (mach_syms)
		    {
		      sp->defined = N_SECT | N_EXT;
		      sp->sect = 3;
		    }
		  else
		    {
		      sp->defined = N_BSS | N_EXT;
		      sp->sect = 0;
		    }
		  bss_size += com;
		  if (write_map)
		    printf ("Allocating common %s: %x at %x\n",
			    sp->name, com, sp->value);
		}
	    }
	}
    }
#ifdef SHLIB
  if (fatal_errors)
    exit(1);
#endif SHLIB

  /* Now with the bss size adjusted for the allocated common symbols the address
     of the objective-C segment can be set, then relocate_file_addresses() can
     be called to set the address of sections and symbols. */
  if (magic == NMAGIC || magic == ZMAGIC ||
      (magic == MH_MAGIC && filetype ==  MH_EXECUTE) || shlib_create)
    {
      objc_start = round(data_start + data_size + bss_size, page_size);
      objc_size = round(sym_size + mod_size + sel_size, page_size);
    }
  else
    {
      objc_start = data_start + data_size + bss_size;
      objc_size = sym_size + mod_size + sel_size;
    }

  /* Compute start addresses of each file's sections and symbols.  */
  each_full_file (relocate_file_addresses, 0);

  /* Now, for each symbol, verify that it is defined globally at most once.
     Put the global value into the symbol entry.  Each defined symbol is
     given a '->defined' field which is the correct N_ code for its definition,
     (note commons are handled in the above loop).  Then make all the references
     point at the symbol entry instead of being chained together. */

  defined_global_sym_count = 0;

  for (i = 0; i < TABSIZE; i++)
    {
      register symbol *sp;
      for (sp = symtab[i]; sp; sp = sp->link)
	{
	  register struct nlist *p, *next;
	  int defs = 0, com = sp->max_common_size, erred = 0;
	  for (p = sp->refs; p; p = next)
	    {
	      register int type = p->n_type;
	      if ((type & N_EXT) && type != (N_UNDF | N_EXT))
		{
		  /* non-common definition */
		  if (defs++ && sp->value != p->n_value)
		    if (!erred++)
		      {
		        make_executable = 0;
			error ("multiple definitions of symbol %s", sp->name);
		      }
		  sp->value = p->n_value;
		  sp->defined = type;
		  sp->sect = p->n_sect;
		}
	      next = (struct nlist *) p->n_un.n_name;
	      p->n_un.n_name = (char *) sp;
	    }
	  /* Note commons already defined (see above) */

	  if (sp->defined)
	    defined_global_sym_count++;
	}
    }

#ifdef SHLIB
  if ((!relocatable_output || force_common_definition) && !shlib_create) {
#else
  if (!relocatable_output || force_common_definition) {
#endif SHLIB
    if (objc_bind_sym.sp)
      {
	objc_bind_sym.sp->value = data_start + objc_bind_sym.data_offset;
	if (mach_syms)
	  {
	    objc_bind_sym.sp->defined = N_SECT | N_EXT;
	    objc_bind_sym.sp->sect = 2;
	  }
	else
	  {
	    objc_bind_sym.sp->defined = N_DATA | N_EXT;
	    objc_bind_sym.sp->sect = 0;
	  }
	undefined_global_sym_count--;
	defined_global_sym_count++;
      }
    if (objc_modDesc_sym.sp)
      {
	objc_modDesc_sym.sp->value =
				data_start + objc_modDesc_sym.ind_tab_offset;
	objc_modDesc_sym.tab_value = data_start + objc_modDesc_sym.tab_offset;
	if (mach_syms)
	  {
	    objc_modDesc_sym.sp->defined = N_SECT | N_EXT;
	    objc_modDesc_sym.sp->sect = 2;
	  }
	else
	  {
	    objc_modDesc_sym.sp->defined = N_DATA | N_EXT;
	    objc_modDesc_sym.sp->sect = 0;
	  }
	undefined_global_sym_count--;
	defined_global_sym_count++;
      }
    if (global_syminfo.sp) {
      global_syminfo.sp->value = data_start + global_syminfo.data_offset;
      if (mach_syms)
	{
	  global_syminfo.sp->defined = N_SECT | N_EXT;
	  global_syminfo.sp->sect = 2;
	}
      else
	{
	  global_syminfo.sp->defined = N_DATA | N_EXT;
	  global_syminfo.sp->sect = 0;
	}
      undefined_global_sym_count--;
      defined_global_sym_count++;
    }
#ifdef SHLIB
    if (shlib_info.sp)
      {
	shlib_info.sp->value = data_start + shlib_info.data_offset;
	if (mach_syms)
	  {
	    shlib_info.sp->defined = N_SECT | N_EXT;
	    shlib_info.sp->sect = 2;
	  }
	else
	  {
	    shlib_info.sp->defined = N_DATA | N_EXT;
	    shlib_info.sp->sect = 0;
	  }
	undefined_global_sym_count--;
	defined_global_sym_count++;
      }
    if (shlib_init.sp)
      {
	shlib_init.sp->value = data_start + shlib_init.data_offset;
	if (mach_syms)
	  {
	    shlib_init.sp->defined = N_SECT | N_EXT;
	    shlib_init.sp->sect = 2;
	  }
	else
	  {
	    shlib_init.sp->defined = N_DATA | N_EXT;
	    shlib_init.sp->sect = 0;
	  }
	undefined_global_sym_count--;
	defined_global_sym_count++;
      }
#endif SHLIB
    if (!Aflag)
      if (mach_syms)
	{
	  set_ld_defined_sym(_ETEXT, text_start + text_size - text_pad,
			     N_SECT | N_EXT, 1);
	  set_ld_defined_sym(_EDATA, data_start + data_size,
			     N_SECT | N_EXT, 2);
	  set_ld_defined_sym(_END, data_start + data_size + bss_size, 
			     N_SECT | N_EXT, 3);
	}
      else
	{
	  set_ld_defined_sym(_ETEXT, text_start + text_size - text_pad,
			     N_TEXT | N_EXT, 0);
	  set_ld_defined_sym(_EDATA, data_start + data_size, N_DATA | N_EXT, 0);
	  set_ld_defined_sym(_END, data_start + data_size + bss_size, 
			     N_BSS | N_EXT, 0);
	}
  }

  /* Figure the data_pad now, so that it overlaps with the bss addresses.  */

  if (specified_data_size && specified_data_size > data_size)
    data_pad = specified_data_size - data_size;

  if (magic == ZMAGIC || (magic == MH_MAGIC && filetype == MH_EXECUTE) ||
      shlib_create)
    data_pad = ((data_pad + data_size + page_size - 1) & (- page_size))
               - data_size;

#ifdef MACH_O
  if (magic == MH_MAGIC)
    {
      dsp->size = data_size;
      bsp->size = bss_size;
    }
#endif MACH_O

  bss_size -= data_pad;
  if (bss_size < 0) bss_size = 0;

  data_size += data_pad;

  if (T_flag_specified || Tdata_flag_specified)
    {
      if (text_start > data_start)
	{
	  if (data_start + data_size + bss_size > text_start)
	    fatal ("text segment (-Ttext = 0x%x size = 0x%x) overlaps with "
		   "data segment (-Tdata = 0x%x size = 0x%x)", text_start,
		   text_size, data_start, data_size + bss_size);
	  if (objc_start + objc_size > text_start)
	    fatal ("text segment (-Ttext = 0x%x size = 0x%x) overlaps with "
		   "objc segment (address = 0x%x size = 0x%x)", text_start,
		   text_size, objc_start, objc_size);
	}
      else
	{
	  if (text_start + text_size > data_start)
	    fatal ("text segment (-Ttext = 0x%x size = 0x%x) overlaps with "
		   "data segment (-Tdata = 0x%x size = 0x%x)", text_start,
		   text_size, data_start, data_size + bss_size);
	}
    }

#ifdef MACH_O
  /* Assign the addresses to the segments created from files. */
  if (n_seg_create > 0 || linkedit_segment)
    {
      long i, j, vmaddr, s_addr;
      struct section_list *slp;

	vmaddr = objc_start + objc_size;
	for (i = 0; i < n_seg_create; i++)
	  {
	    seg_create[i].sg.vmaddr = vmaddr;
	    s_addr = vmaddr;
	    slp = seg_create[i].slp;
	    for (j = 0; j < seg_create[i].sg.nsects; j++)
	      {
		slp->s.addr = s_addr;
		s_addr += slp->s.size;
		slp = slp->next;
	      }
	    vmaddr += seg_create[i].sg.vmsize;
	  }
	if (linkedit_segment)
	  {
	    linkedit_segment->vmaddr = vmaddr;
	  }
    }

  /* Note the data or objc can't overlap with a segment created from a file */
  for (i = 0; i < n_seg_create; i++)
    {
      check_overlap(seg_create[i].sg.vmaddr, seg_create[i].sg.vmsize,
		    output_filename, seg_create[i].sg.segname,
  		    text_start, text_size, output_filename, SEG_TEXT);
    }

  for (i = 0; i < n_fvmlibs; i++)
    {
      check_overlap(fvmlibs[i].shlib.text_addr, fvmlibs[i].shlib.text_size,
		    fvmlibs[i].shlib.name, SEG_TEXT,
      		    fvmlibs[i].shlib.data_addr, fvmlibs[i].shlib.data_size,
		    fvmlibs[i].shlib.name, SEG_DATA);

      check_overlap(fvmlibs[i].shlib.text_addr, fvmlibs[i].shlib.text_size,
		    fvmlibs[i].shlib.name, SEG_TEXT,
  		    text_start, text_size, output_filename, SEG_TEXT);
      check_overlap(fvmlibs[i].shlib.text_addr, fvmlibs[i].shlib.text_size,
		    fvmlibs[i].shlib.name, SEG_TEXT,
		    data_start, data_size + bss_size,
		    output_filename, SEG_DATA);
      check_overlap(fvmlibs[i].shlib.text_addr, fvmlibs[i].shlib.text_size,
		    fvmlibs[i].shlib.name, SEG_TEXT,
  		    objc_start, objc_size, output_filename, SEG_OBJC);

      check_overlap(fvmlibs[i].shlib.data_addr, fvmlibs[i].shlib.data_size,
		    fvmlibs[i].shlib.name, SEG_DATA,
  		    text_start, text_size, output_filename, SEG_TEXT);
      check_overlap(fvmlibs[i].shlib.data_addr, fvmlibs[i].shlib.data_size,
		    fvmlibs[i].shlib.name, SEG_DATA,
		    data_start, data_size + bss_size,
		    output_filename, SEG_DATA);
      check_overlap(fvmlibs[i].shlib.data_addr, fvmlibs[i].shlib.data_size,
		    fvmlibs[i].shlib.name, SEG_DATA,
  		    objc_start, objc_size, output_filename, SEG_OBJC);
      for (j = 0; j < n_seg_create; j++)
	{
	  check_overlap(fvmlibs[i].shlib.text_addr, fvmlibs[i].shlib.text_size,
			fvmlibs[i].shlib.name, SEG_TEXT,
			seg_create[j].sg.vmaddr, seg_create[j].sg.vmsize,
			output_filename, seg_create[j].sg.segname);
	  check_overlap(fvmlibs[i].shlib.data_addr, fvmlibs[i].shlib.data_size,
			fvmlibs[i].shlib.name, SEG_DATA,
			seg_create[j].sg.vmaddr, seg_create[j].sg.vmsize,
			output_filename, seg_create[j].sg.segname);
	}
      for (j = i+1; j < n_fvmlibs; j++)
	{
	  check_overlap(fvmlibs[i].shlib.text_addr, fvmlibs[i].shlib.text_size,
			fvmlibs[i].shlib.name, SEG_TEXT,
			fvmlibs[j].shlib.text_addr, fvmlibs[j].shlib.text_size,
			fvmlibs[j].shlib.name, SEG_TEXT);
	  check_overlap(fvmlibs[i].shlib.text_addr, fvmlibs[i].shlib.text_size,
			fvmlibs[i].shlib.name, SEG_TEXT,
			fvmlibs[j].shlib.data_addr, fvmlibs[j].shlib.data_size,
			fvmlibs[j].shlib.name, SEG_DATA);

	  check_overlap(fvmlibs[i].shlib.data_addr, fvmlibs[i].shlib.data_size,
			fvmlibs[i].shlib.name, SEG_DATA,
			fvmlibs[j].shlib.text_addr, fvmlibs[j].shlib.text_size,
			fvmlibs[j].shlib.name, SEG_TEXT);
	  check_overlap(fvmlibs[i].shlib.data_addr, fvmlibs[i].shlib.data_size,
			fvmlibs[i].shlib.name, SEG_DATA,
			fvmlibs[j].shlib.data_addr, fvmlibs[j].shlib.data_size,
			fvmlibs[j].shlib.name, SEG_DATA);
	}
    }

  if (fatal_errors)
    exit(1);
#endif MACH_O
}

check_overlap(addr1, size1, file1, seg1, addr2, size2, file2, seg2)
    long addr1, size1, addr2, size2;
    char *file1, *seg1, *file2, *seg2;
{
  if (addr1 > addr2){
      if (addr2 + size2 <= addr1)
	return;
  }
  else{
      if (addr1 + size1 <= addr2)
	return;
  }

  fatal_error ("%s segment (address = 0x%x size = 0x%x) of %s overlaps "
	       "with %s segment (address = 0x%x size = 0x%x) of %s",
	       seg1, addr1, size1, file1, seg2, addr2, size2, file2);
}

/* Accumulate the section sizes of input file ENTRY
   into the section sizes of the output file.  */


void
consider_file_section_lengths (entry)
     register struct file_entry *entry;
{
  if (entry->just_syms_flag)
    return;

  data_size = round(data_size, sizeof(long));
  bss_size = round(bss_size, sizeof(long));
  sym_size = round(sym_size, sizeof(long));
  mod_size = round(mod_size, sizeof(long));

  entry->text_start_address = text_size;
  text_size += entry->header.a_text;
  entry->data_start_address = data_size;
  data_size += entry->header.a_data;
#ifdef SHLIB
  if (shlib_create)
    {
      entry->global_data_start_address = global_data_size;
      global_data_size += (entry->header.a_text + entry->header.a_data) -
			  entry->first_global_data_address;
      entry->static_data_start_address = static_data_size;
      static_data_size += entry->first_global_data_address -
			  entry->header.a_text;
    }
#endif SHLIB
  entry->bss_start_address = bss_size;
  bss_size += entry->header.a_bss;
  entry->sym_start_address = sym_size;
  sym_size += entry->sym_size;
  entry->mod_start_address = mod_size;
  mod_size += entry->mod_size;

  text_reloc_size += entry->header.a_trsize;
  data_reloc_size += entry->header.a_drsize;
  sym_reloc_size += entry->sym_reloc_size;
  mod_reloc_size += entry->mod_reloc_size;
  sel_reloc_size += entry->sel_reloc_size;
}

/* Determine where the sections of ENTRY go into the output file,
   whose total section sizes are already known.
   Also relocate the addresses of the file's local and debugger symbols.  */

void
relocate_file_addresses (entry)
     register struct file_entry *entry;
{
  register struct nlist *p, *end;
  register int type;

  /* Note that `data_size' has not yet been adjusted for `data_pad'.  If it
     had been, we would get the wrong results here.  But commom symbols have
     been defined so that objc_start was set correctly to the page after the
     data segment. */

  entry->text_start_address += text_start;
  entry->data_start_address += data_start;
#ifdef SHLIB
  if(shlib_create)
    {
      entry->global_data_start_address += global_data_start;
      entry->static_data_start_address += static_data_start;
    }
#endif SHLIB
  entry->bss_start_address += data_start + data_size;
  entry->sym_start_address += objc_start;
  entry->mod_start_address += objc_start + sym_size;

  end = entry->symbols + entry->header.a_syms / sizeof (struct nlist);
  for (p = entry->symbols; p < end; p++)
    {
      /* If this symbol belongs to a section, update it by the section's 
	 start address */
      type = p->n_type & N_TYPE;

      if(type == N_SECT || (p->n_type & N_STAB) != 0)
	{
	  switch(p->n_sect)
	    {
	      case NO_SECT:
		if((p->n_type & N_STAB) == 0)
		  fatal_with_file (entry, "Defined symbol (%d) not in any "
				   "section (n_sect is NO_SECT) in: ", 
				   (p - entry->symbols));
		break;
	      case 1:
		p->n_value += entry->text_start_address;
		break;
	      case 2:
#ifdef SHLIB
		if(shlib_create)
		  {
		    if(p->n_value < entry->first_global_data_address)
		      p->n_value += entry->static_data_start_address -
				    entry->header.a_text;
		    else
		      p->n_value += entry->global_data_start_address -
				    entry->first_global_data_address;
		  }
		else
#endif SHLIB
		p->n_value += entry->data_start_address - entry->header.a_text;
		break;
	      case 3:
		p->n_value += entry->bss_start_address - entry->bss_addr;
		break;
	      case 4:
		p->n_value += entry->sym_start_address - entry->sym_addr;
		break;
	      case 5:
		p->n_value += entry->mod_start_address - entry->mod_addr;
		break;
	      case 6:
		p->n_value = objc_start + sym_size + mod_size +
		     map_selector_offset (entry, p->n_value - entry->sel_addr);
		break;
	      default:
		fatal_with_file (entry, "Unknown section (%d) for symbol (%d) "
				 "in: ", p->n_sect, (p - entry->symbols));
	    }
	}
      else if (type == N_TEXT)
	p->n_value += entry->text_start_address;
      else if (type == N_DATA)
	{
#ifdef SHLIB
	  if(shlib_create)
	    {
	      if(p->n_value < entry->first_global_data_address)
		p->n_value += entry->static_data_start_address -
			      entry->header.a_text;
	      else
		p->n_value += entry->global_data_start_address -
			      entry->first_global_data_address;
	    }
	  else
#endif SHLIB
	    /* A symbol whose value is in the data section
	       is present in the input file as if the data section
	       started at an address equal to the length of the file's text.*/
	    p->n_value += entry->data_start_address - entry->header.a_text;
	}
      else if (type == N_BSS)
	/* likewise for symbols with value in BSS.  */
	p->n_value += entry->bss_start_address - entry->bss_addr;
    }
}

void describe_file_sections (), list_file_locals ();

/* Print a complete or partial map of the output file.  */

void
print_symbols (outfile)
     FILE *outfile;
{
  register int i;

  fprintf (outfile, "\nFiles:\n\n");

  each_file (describe_file_sections, outfile);

  fprintf (outfile, "\nGlobal symbols:\n\n");

  for (i = 0; i < TABSIZE; i++)
    {
      register symbol *sp;
      for (sp = symtab[i]; sp; sp = sp->link)
	{
	  if (sp->defined == 1)
	    fprintf (outfile, "  %s: common, length 0x%x\n", sp->name, sp->max_common_size);
	  if (sp->defined)
	    fprintf (outfile, "  %s: 0x%x\n", sp->name, sp->value);
	  else if (sp->referenced)
	    fprintf (outfile, "  %s: undefined\n", sp->name);
	}
    }

  each_file (list_file_locals, outfile);
}

void
describe_file_sections (entry, outfile)
     struct file_entry *entry;
     FILE *outfile;
{
  fprintf (outfile, "  ");
  print_file_name (entry, outfile);
  if (entry->just_syms_flag)
    fprintf (outfile, " symbols only\n", 0);
  else
    {
      fprintf (outfile, " text %x(%x), data %x(%x), bss %x(%x)",
	       entry->text_start_address, entry->header.a_text,
	       entry->data_start_address, entry->header.a_data,
	       entry->bss_start_address, entry->header.a_bss);
      fprintf (outfile, " sym %x(%x), mod %x(%x), sel (%x) hex\n",
	       entry->sym_start_address, entry->sym_size,
	       entry->mod_start_address, entry->mod_size,
	       entry->sel_size);
    }
}

void
list_file_locals (entry, outfile)
     struct file_entry *entry;
     FILE *outfile;
{
  register struct nlist *p, *end = entry->symbols + entry->header.a_syms / sizeof (struct nlist);

  entry->strings = (char *) xmalloc (entry->string_size);
  read_entry_strings (file_open (entry), entry);

  fprintf (outfile, "\nLocal symbols of ");
  print_file_name (entry, outfile);
  fprintf (outfile, ":\n\n");

  for (p = entry->symbols; p < end; p++)
    {
      if (!(p->n_type & (N_STAB | N_EXT)))
	if (p->n_un.n_strx < 0 || p->n_un.n_strx > entry->string_size)
	  fatal_with_file (entry, "Bad string table index (%d) for symbol "
			   "(%d) in: ", p->n_un.n_strx, (p - entry->symbols));
	else
	  fprintf (outfile, "  %s: 0x%x\n",
		   entry->strings + p->n_un.n_strx, p->n_value);
    }
  free (entry->strings);
  entry->strings = NULL;
}

/*
 * These two routines set the values of the link editor defined symbols
 * for the symbols for the begining and end of a segment created from a
 * file if they are used.  It is an error to use these an not let the
 * link editor define their values.
 */
void
ld_defined_symbols()
{
  register int i, j, sect;
  struct section_list *slp;

  if ((relocatable_output && !force_common_definition) ||
      (magic == MH_MAGIC && filetype == MH_FVMLIB))
    return;

  if (magic == MH_MAGIC && filetype == MH_EXECUTE && !Aflag)
    if (mach_syms)
      set_ld_defined_sym(_MH_EXECUTE_SYM, text_start, N_SECT | N_EXT, 1);
    else
      set_ld_defined_sym(_MH_EXECUTE_SYM, text_start, N_TEXT | N_EXT, 0);

  if (!Aflag)
    {
      if (mach_syms)
	{
	  if (magic == MH_MAGIC)
	    {
	      set_ld_defined_sym(SEG_TEXT "__begin", text_start,
				 N_SECT | N_EXT, 1);
	      if (filetype == MH_OBJECT || filetype == MH_PRELOAD)
		{
		  set_ld_defined_sym(SEG_TEXT SECT_TEXT "__begin",
				     text_start,
				     N_SECT | N_EXT, 1);
		  set_ld_defined_sym(SEG_TEXT SECT_TEXT "__end",
				     text_start + tsp->size,
				     N_SECT | N_EXT, 1);
		}
	      else
		{
		  set_ld_defined_sym(SEG_TEXT SECT_TEXT "__begin",
				     text_start + mach_headers_size,
				     N_SECT | N_EXT, 1);
		  set_ld_defined_sym(SEG_TEXT SECT_TEXT "__end",
				     text_start + mach_headers_size + tsp->size,
				     N_SECT | N_EXT, 1);
		}
	      set_ld_defined_sym(SEG_TEXT "__end", text_start + text_size,
				 N_SECT | N_EXT, 1);

	      set_ld_defined_sym(SEG_DATA "__begin", data_start,
				 N_SECT | N_EXT, 2);
	      set_ld_defined_sym(SEG_DATA SECT_DATA "__begin", data_start,
				 N_SECT | N_EXT, 2);
	      set_ld_defined_sym(SEG_DATA SECT_DATA "__end",
				 data_start + dsp->size,
				 N_SECT | N_EXT, 2);
	      set_ld_defined_sym(SEG_DATA SECT_BSS "__begin",
				 data_start + dsp->size,
				 N_SECT | N_EXT, 3);
	      set_ld_defined_sym(SEG_DATA SECT_BSS "__end",
				 data_start + dsp->size + bsp->size,
				 N_SECT | N_EXT, 3);
	      set_ld_defined_sym(SEG_DATA "__end", data_start + 
	  		(data_size + bss_size + page_size - 1) & (- page_size),
				 N_SECT | N_EXT, 3);
	    }
	}
      else
	{
	  if (magic == MH_MAGIC)
	    {
	      set_ld_defined_sym(SEG_TEXT "__begin", text_start,
				 N_TEXT | N_EXT, 0);
	      if (filetype == MH_OBJECT || filetype == MH_PRELOAD)
		{
		  set_ld_defined_sym(SEG_TEXT SECT_TEXT "__begin",
				     text_start,
				     N_TEXT | N_EXT, 0);
		  set_ld_defined_sym(SEG_TEXT SECT_TEXT "__end",
				     text_start + tsp->size,
				     N_TEXT | N_EXT, 0);
		}
	      else
		{
		  set_ld_defined_sym(SEG_TEXT SECT_TEXT "__begin",
				     text_start + mach_headers_size,
				     N_TEXT | N_EXT, 0);
		  set_ld_defined_sym(SEG_TEXT SECT_TEXT "__end",
				     text_start + mach_headers_size + tsp->size,
				     N_TEXT | N_EXT, 0);
		}
	      set_ld_defined_sym(SEG_TEXT "__end", text_start + text_size,
				 N_TEXT | N_EXT, 0);

	      set_ld_defined_sym(SEG_DATA "__begin", data_start,
				 N_DATA | N_EXT, 0);
	      set_ld_defined_sym(SEG_DATA SECT_DATA "__begin", data_start,
				 N_DATA | N_EXT, 0);
	      set_ld_defined_sym(SEG_DATA SECT_DATA "__end",
				 data_start + dsp->size,
				 N_DATA | N_EXT, 0);
	      set_ld_defined_sym(SEG_DATA SECT_BSS "__begin",
				 data_start + dsp->size,
				 N_BSS | N_EXT, 0);
	      set_ld_defined_sym(SEG_DATA SECT_BSS "__end",
				 data_start + dsp->size + bsp->size,
				 N_BSS | N_EXT, 0);
	      set_ld_defined_sym(SEG_DATA "__end", data_start + data_size,
				 N_BSS | N_EXT, 0);
	    }
	}

      /* If these are used they are always set as Mach-O symbols */
      set_ld_defined_sym(SEG_OBJC "__begin", objc_start,
			 N_SECT | N_EXT, 4);
      set_ld_defined_sym(SEG_OBJC SECT_OBJC_SYMBOLS "__begin", objc_start,
			 N_SECT | N_EXT, 4);
      set_ld_defined_sym(SEG_OBJC SECT_OBJC_SYMBOLS "__end",
			 objc_start + sym_size,
			 N_SECT | N_EXT, 4);
      set_ld_defined_sym(SEG_OBJC SECT_OBJC_MODULES "__begin",
			 objc_start + sym_size,
			 N_SECT | N_EXT, 5);
      set_ld_defined_sym(SEG_OBJC SECT_OBJC_MODULES "__end",
			 objc_start + sym_size + mod_size,
			 N_SECT | N_EXT, 5);
      set_ld_defined_sym(SEG_OBJC SECT_OBJC_STRINGS "__begin",
			 objc_start + sym_size + mod_size,
			 N_SECT | N_EXT, 6);
      set_ld_defined_sym(SEG_OBJC SECT_OBJC_STRINGS "__end",
			 objc_start + sym_size + mod_size + sel_size,
			 N_SECT | N_EXT, 6);
      set_ld_defined_sym(SEG_OBJC "__end", objc_start + objc_size,
			   N_SECT | N_EXT, 6);
    }

  sect = 7;
  if(filetype == MH_PRELOAD)
    for (i = 0; i < n_seg_create; i++)
      {
	if (mach_syms)
	    set_ld_defined_sym(seg_create[i].begin, seg_create[i].sg.vmaddr,
			       N_SECT | N_EXT, sect);
	else
	    set_ld_defined_sym(seg_create[i].begin, seg_create[i].sg.vmaddr,
			       N_ABS | N_EXT, 0);
	slp = seg_create[i].slp;
	for (j = 0; j < seg_create[i].sg.nsects; j++)
	  {
	    if (mach_syms)
	      {
		set_ld_defined_sym(slp->begin, slp->s.addr,
				   N_SECT | N_EXT, sect);
		set_ld_defined_sym(slp->end, slp->s.addr + slp->s.size,
				   N_SECT | N_EXT, sect);
		sect++;
	      }
	    else
	      {
		set_ld_defined_sym(slp->begin, slp->s.addr,
				   N_ABS | N_EXT, 0);
		set_ld_defined_sym(slp->end, slp->s.addr + slp->s.size,
				   N_ABS | N_EXT, 0);
	      }
	  }
	if (mach_syms)
	    set_ld_defined_sym(seg_create[i].end,
			       seg_create[i].sg.vmaddr +seg_create[i].sg.vmsize,
			       N_SECT | N_EXT, sect - 1);
	else
	    set_ld_defined_sym(seg_create[i].end,
			       seg_create[i].sg.vmaddr +seg_create[i].sg.vmsize,
			       N_ABS | N_EXT, 0);
      }
}

void
set_ld_defined_sym(name, value, type, sect)
char *name;
long value;
long type;
long sect;
{
  register symbol *sp;

  sp = getsym_soft(name);
  if (sp && sp->referenced)
    if (sp->defined)
      error("user attempt to redefine loader-defined symbol %s\n", name);
    else
      {
	undefined_global_sym_count--;
	defined_global_sym_count++;
	sp->defined = type;
	sp->value = value;
	sp->sect = sect;
      }
}

/*
 * This routine creates a linked list of symbol pairs to be used as the
 * objective C module description table if it is referenced (pre 1.0 style).
 */
void
objc_bind ()
{
  long info_name_len, new_len, i;
  char *info_name;
  struct bindlist *head, *next;
  symbol *sp, *info_sp;

  objc_bind_sym.sp = getsym_soft(_OBJC_BIND_SYM);
  if (!objc_bind_sym.sp)
    return;
  if (objc_bind_sym.sp->defined)
    {
      if (!Aflag)
	{
	  error("user attempt to redefine loader-defined symbol %s\n",
		_OBJC_BIND_SYM);
	  objc_bind_sym.sp = (symbol *)0;
	}
      return;
    }

  info_name_len = sizeof(_OBJC_INFO) + 1;
  info_name = (char *) xmalloc (info_name_len);
  strcpy(info_name, _OBJC_INFO);

  head = (struct bindlist *) xmalloc (sizeof(struct bindlist));
  head->link = (symbol *)0;
  head->info = (symbol *)0;
  head->next = (struct bindlist *)0;

  objc_bind_sym.n_bindlist = 1;

  for (i = 0; i < TABSIZE; i++)
    {
      for (sp = symtab[i]; sp; sp = sp->link)
	{
	  if (strncmp(sp->name, _OBJC_LINK, sizeof(_OBJC_LINK)-1) == 0)
	    {
	      next = (struct bindlist *) xmalloc (sizeof(struct bindlist));
	      next->link = (symbol *)0;
	      next->info = (symbol *)0;
	      next->next = head;
	      head = next;
	      objc_bind_sym.n_bindlist++;
	      if (!sp->defined)
		error("Objective C module link symbol %s not defined\n",
		       sp->name);
	      else
		next->link = sp;
	      new_len = strlen(sp->name) + sizeof(_OBJC_INFO);
	      if(new_len > info_name_len)
		{
		  info_name = (char *) xrealloc (info_name, new_len);
		  info_name_len = new_len;
		  strcpy(info_name, _OBJC_INFO);
		}
	      info_name[sizeof(_OBJC_INFO) - 1] = '\0';
	      strcat(info_name, sp->name + sizeof(_OBJC_LINK) - 1);
	      info_sp = getsym_soft(info_name);
	      if (!info_sp || !info_sp->defined)
		error("Objective C module Info symbol %s not defined\n",
		       info_name);
	      else
		next->info = info_sp;
	    }
	}
    }
  objc_bind_sym.bindlist = head;
  data_size = round (data_size, sizeof(long));
  objc_bind_sym.data_offset = data_size;
  data_size += objc_bind_sym.n_bindlist * 2 * sizeof(long);
  free (info_name);
}

/*
 * This routine creates a linked list of the symbols to be used as the
 * objective C module description indirect table if it is referenced and
 * the single module descriptor table (1.0 and later style).
 */
void
objc_modDesc ()
{
  long i;
  struct symlist *new;
  symbol *sp;

  objc_modDesc_sym.sp = getsym_soft(_MOD_DESC_IND_TAB);
  if (!objc_modDesc_sym.sp)
    return;
  if (objc_modDesc_sym.sp->defined)
    {
      if (!Aflag)
	{
	  error("user attempt to redefine loader-defined symbol %s\n",
		_MOD_DESC_IND_TAB);
	  objc_modDesc_sym.sp = (symbol *)0;
	}
      return;
    }

  objc_modDesc_sym.tablelist =
			(struct symlist *) xmalloc (sizeof(struct symlist));
  objc_modDesc_sym.tablelist->sp = NULL;
  objc_modDesc_sym.tablelist->next = NULL;
  objc_modDesc_sym.n_tables = 1;

  objc_modDesc_sym.desclist =
			(struct symlist *) xmalloc (sizeof(struct symlist));
  objc_modDesc_sym.desclist->sp = NULL;
  objc_modDesc_sym.desclist->next = NULL;
  objc_modDesc_sym.n_descs = 1;

  for (i = 0; i < TABSIZE; i++)
    {
      for (sp = symtab[i]; sp; sp = sp->link)
	{
	  if (strncmp(sp->name, _MOD_DESC_TAB, sizeof(_MOD_DESC_TAB)-1) == 0)
	    {
	      if (!sp->defined)
		error("Objective C module descriptor table symbol %s not "
		      "defined\n", sp->name);
	      else
		{
		  new = (struct symlist *) xmalloc (sizeof(struct symlist));
		  new->sp = sp;
		  new->next = objc_modDesc_sym.tablelist;
		  objc_modDesc_sym.tablelist = new;
		  objc_modDesc_sym.n_tables++;
		}
	    }
	  if (strncmp(sp->name, _MOD_DESC, sizeof(_MOD_DESC)-1) == 0)
	    {
	      if (!sp->defined)
		error("Objective C module descriptor symbol %s not "
		      "defined\n", sp->name);
	      else
		/*
		 * It would be nice here to only record the symbols that are
		 * not N_ABS symbols but that is not known at this point.
		 */
		  {
		    new = (struct symlist *) xmalloc (sizeof(struct symlist));
		    new->sp = sp;
		    new->next = objc_modDesc_sym.desclist;
		    objc_modDesc_sym.desclist = new;
		    objc_modDesc_sym.n_descs++;
		  }
	    }
	}
    }

  data_size = round (data_size, sizeof(long));
  objc_modDesc_sym.ind_tab_offset = data_size;
  /* the + 1 is for the single module descriptor table sized next */
  data_size += (objc_modDesc_sym.n_tables + 1) * sizeof(long);

  objc_modDesc_sym.tab_offset = data_size;
  data_size += objc_modDesc_sym.n_descs * sizeof(long);
}

void
gen_global_symtab_info()
{
  register i;
  symbol *sp, *etext_sp, *edata_sp, *end_sp;

  global_syminfo.sp = getsym_soft(LOAD_GLOBAL_SYMTAB_INFO);
  if (!global_syminfo.sp)
    return;
  if (global_syminfo.sp->defined)
    {
      if (!Aflag)
	{
	  error("user attempt to redefine loader-defined symbol %s\n",
		LOAD_GLOBAL_SYMTAB_INFO);
	  global_syminfo.sp = (symbol *)0;
	}
      return;
    }
  
  etext_sp = getsym_soft(_ETEXT);
  edata_sp = getsym_soft(_EDATA);
  end_sp   = getsym_soft(_END);

  global_syminfo.data_offset = data_size;
  global_syminfo.n_globals = 0;
  global_syminfo.symtabsize = 4;

  for(i=0;i<TABSIZE;i++)
    for(sp = symtab[i]; sp; sp = sp->link)
      if (sp->defined & N_EXT || sp == objc_bind_sym.sp ||
	  sp == objc_modDesc_sym.sp || sp == global_syminfo.sp ||
	  sp == shlib_info.sp || sp == shlib_init.sp ||
	  sp == etext_sp || sp == edata_sp || sp == end_sp) {
	register size;

	/* Size = length of string + null + symtype byte */
	size = strlen(sp->name) + 2;
	size = round(size, sizeof(long));
	global_syminfo.symtabsize += size + sizeof(long);
	global_syminfo.n_globals++;
      }
  data_size += global_syminfo.symtabsize;
}

#ifdef SHLIB
/*
 * This routine creates the linked list of symbols to be used for the
 * shared library information and initialization tables.
 */
void
shlib_syms ()
{
  long i;
  struct symlist *head, *next;
  symbol *sp;

  shlib_info.sp = getsym_soft(_SHLIB_INFO);
  if (shlib_info.sp && shlib_info.sp->defined)
    {
      if (!Aflag)
	{
	  error("user attempt to redefine loader-defined symbol %s\n",
		_SHLIB_INFO);
	  shlib_info.sp = (symbol *)0;
	}
      return;
    }
  shlib_init.sp = getsym_soft(_SHLIB_INIT);
  if (shlib_init.sp && shlib_init.sp->defined)
    {
      if (!Aflag)
	{
	  error("user attempt to redefine loader-defined symbol %s\n",
		_SHLIB_INIT);
	  shlib_init.sp = (symbol *)0;
	  shlib_info.sp = (symbol *)0;
	}
      return;
    }

  /*
   * Collect the symbols for the shared library information table.
   */
  if(shlib_info.sp)
    {
      head = (struct symlist *) xmalloc (sizeof(struct symlist));
      head->sp = (symbol *)0;
      head->next = (struct symlist *)0;

      shlib_info.n_symlist = 1;

      for (i = 0; i < TABSIZE; i++)
	{
	  for (sp = symtab[i]; sp; sp = sp->link)
	    {
	      if (strncmp(sp->name, SHLIB_SYMBOL_NAME,
			  sizeof(SHLIB_SYMBOL_NAME) - 1) == 0)
		{
		  next = (struct symlist *) xmalloc (sizeof(struct symlist));
		  next->sp = (symbol *)0;
		  next->next = head;
		  head = next;
		  shlib_info.n_symlist++;
		  if (!sp->defined)
		    error("Shared library information symbol %s not defined\n",
			   sp->name);
		  else
		    next->sp = sp;
		}
	    }
	}
	shlib_info.symlist = head;
	data_size = round (data_size, sizeof(long));
	shlib_info.data_offset = data_size;
	data_size += shlib_info.n_symlist * 1 * sizeof(long);
    }

  /*
   * Collect the symbols for the shared library initialization table.
   */
  if(shlib_init.sp)
    {
      head = (struct symlist *) xmalloc (sizeof(struct symlist));
      head->sp = (symbol *)0;
      head->next = (struct symlist *)0;

      shlib_init.n_symlist = 1;

      for (i = 0; i < TABSIZE; i++)
	{
	  for (sp = symtab[i]; sp; sp = sp->link)
	    {
	      if (strncmp(sp->name, INIT_SYMBOL_NAME,
			  sizeof(INIT_SYMBOL_NAME) - 1) == 0)
		{
		  next = (struct symlist *) xmalloc (sizeof(struct symlist));
		  next->sp = (symbol *)0;
		  next->next = head;
		  head = next;
		  shlib_init.n_symlist++;
		  if (!sp->defined)
		    error("Shared library initialization symbol %s not "
			  "defined\n", sp->name);
		  else
		    next->sp = sp;
		}
	    }
	}
	shlib_init.symlist = head;
	data_size = round (data_size, sizeof(long));
	shlib_init.data_offset = data_size;
	data_size += shlib_init.n_symlist * 1 * sizeof(long);
    }
}
#endif SHLIB

#ifdef SYM_ALIAS
/*
 * This routine set the values of the aliased symbols from there original
 * symbol value.  The original symbol must be undefined.
 */
void
set_alias_values ()
{
  register int i;
  register symbol *sp, *alias;

  for (i = 0; i < TABSIZE; i++)
    {
      for (sp = symtab[i]; sp; sp = sp->link)
	{
	  if (sp->alias_name)
	    {
	      if (sp->defined)
		fatal ("defined symbol: %s can't be aliased", sp->name);
	      alias = getsym_soft(sp->alias_name);
	      if (!alias)
		fatal ("symbol alias: %s not found", sp->alias_name);
	      /* this information is set here so relocation can be done */
	      sp->value = alias->value;
	      sp->referenced = alias->referenced;
	      sp->defined = alias->defined;
	      sp->sect = alias->sect;
	      /* the aliased symbol use to be undefined */
	      undefined_global_sym_count--;
	    }
	}
    }
}

/*
 * This routine removes symbols that have been aliased from the symbol table
 * so only the alias name will appear in the output.  It puts these removed
 * symbols on a list to be used by write_alias_sym_info.
 */
void
remove_aliased_symbols ()
{
  register int i;
  register symbol *sp, *prev;

  for (i = 0; i < TABSIZE; i++)
    {
      prev = (symbol *)0;
      for (sp = symtab[i]; sp; )
	{
	  if (sp->alias_name)
	    {
	      /* Found an alias name, remove it from the current symbol chain */
	      if (prev)
		prev->link = sp->link;
	      else
		symtab[i] = sp->link;
	      /* Now hang it on the removed_aliased_symbols chain */
	      sp->link = removed_aliased_symbols;
	      removed_aliased_symbols = sp;
	    }
	  /* Wasn't an alias, remember sp in case we need to remove our link. */
	  else
	    prev = sp;
	  if (prev)
            sp = prev->link;
	  else if (symtab[i])
	    sp = symtab[i];
	  else
	    sp = 0;
	}
    }
}
#endif SYM_ALIAS

/* Print on OUTFILE a list of all the undefined global symbols.
   If there are any such, it is an error, so clear `make_executable'.  */

void
list_undefined_symbols (outfile)
     FILE *outfile;
{
  register int i, j, ok;
  register int count = 0;

  for (i = 0; i < TABSIZE; i++)
    {
      register symbol *sp;
      for (sp = symtab[i]; sp; sp = sp->link)
	{
	  if (sp->referenced && !sp->defined)
	    {
	      ok = 0;
	      for (j = 0; j < n_Usymnames; j++)
		{
		  if (strcmp(Usymnames[j], sp->name) == 0)
		    {
		      ok = 1;
		      break;
		    }
		}
	      if (ok)
		continue;
	      if (!count++)
		if (!relocatable_output || force_common_definition)
		  fprintf (outfile, "Undefined symbols:\n");
	      if (!relocatable_output || force_common_definition)
		fprintf (outfile, " %s\n", sp->name);
	    }
	}
    }

  if (count && !relocatable_output)
    fprintf (outfile, "\n");
  if (count)
    make_executable = 0;
#ifdef MACH_O
  if (magic == MH_MAGIC)
    if (count)
      mhp->flags = 0;
    else
      mhp->flags = MH_NOUNDEFS;
#endif MACH_O
}

/* Write the output file */ 

void
write_output ()
{
  struct stat statbuf;
  int filemode;

#ifdef CROSS
  unlink (output_filename);
#endif
  outdesc = open (output_filename, O_WRONLY | O_CREAT | O_TRUNC, 0666);
  if (outdesc < 0)
    perror_fatal ("Can't create output file: %s", output_filename);

  if (fstat (outdesc, &statbuf) < 0)
    perror_fatal ("Can't stat output file: %s", output_filename);

  filemode = statbuf.st_mode & 0777;

  chmod (output_filename, filemode & ~0111);

  /* Output the a.out header.  */
  write_header ();

  /* Output the text and data segments, relocating as we go.  */
  write_text ();
  write_data ();
  write_objc ();

#ifdef MACH_O
  /* Output the segments created from files if any.  */
  if (n_seg_create > 0)
    write_seg_create ();
#endif MACH_O

  /* Output the merged relocation info, if requested with `-r'.  */
  if (relocatable_output)
    write_rel ();

#ifdef SYM_ALIAS
  remove_aliased_symbols ();
#endif SYM_ALIAS

  /* Output the symbol table (both globals and locals).  */
  write_syms ();
 
#ifdef MACH_O
  if (magic == MH_MAGIC && strip_symbols != STRIP_ALL)
    {
#ifndef NO_SYMSEGS
      ssp->offset = lseek (outdesc, 0, L_INCR);
#endif
      stp->strsize = lseek (outdesc, 0, L_INCR) - stp->stroff;
    }
#endif MACH_O

#ifndef NO_SYMSEGS
  /* Copy any GDB symbol segments from input files.  */
  write_symsegs ();
#endif

#ifdef MACH_O
  if (magic == MH_MAGIC)
    {
      int i, j;
      struct section_list *slp;

#ifndef NO_SYMSEGS
      if (strip_symbols != STRIP_ALL)
	{
	  ssp->size = lseek (outdesc, 0, L_INCR) - ssp->offset;
	}
#endif
      if (linkedit_segment)
        {
	  linkedit_segment->filesize = lseek (outdesc, 0, L_INCR) -
				       linkedit_segment->fileoff;
	  linkedit_segment->vmsize = round(linkedit_segment->filesize,
				           page_size);
        }
      lseek (outdesc, 0, L_SET);
      if (filetype == MH_OBJECT)
	mywrite (&rel_mach, sizeof (struct mach_rel), 1, outdesc);
      else
	mywrite (&exec_mach, sizeof (struct mach_aout), 1, outdesc);
      if (pagezero_segment)
        mywrite (pagezero_segment, sizeof (struct segment_command), 1, outdesc);
      if (linkedit_segment)
        mywrite (linkedit_segment, sizeof (struct segment_command), 1, outdesc);
      if (filetype == MH_EXECUTE || filetype == MH_PRELOAD)
        mywrite (&thread, sizeof (struct thread_aout), 1, outdesc);
      for (i = 0; i < n_seg_create; i++)
	{
	  mywrite (&(seg_create[i].sg), sizeof (struct segment_command),
		    1, outdesc);
	  slp = seg_create[i].slp;
	  for (j = 0; j < seg_create[i].sg.nsects; j++)
	    {
	      mywrite (&(slp->s), sizeof (struct section), 1, outdesc);
	      slp = slp->next;
	    }
	}
      if (filetype != MH_OBJECT)
	for (i = 0; i < n_fvmlibs; i++)
	  {
	      mywrite (&(fvmlibs[i].lc_fvmlib),
		       sizeof (struct fvmlib_command), 1, outdesc);
	      mywrite (fvmlibs[i].shlib.name, fvmlibs[i].name_size,
		       1, outdesc);
	  }
      if (ident.strings_size != 0)
	{
	    mywrite (&ident.lc_ident, sizeof (struct ident_command), 1,outdesc);
	    mywrite (ident.strings, ident.lc_ident.cmdsize -
		     sizeof (struct ident_command), 1, outdesc);
	}
    }
#endif MACH_O

  close (outdesc);

  if (make_executable)
    chmod (output_filename, filemode | 0111);
}

void modify_location (), perform_relocation ();
void copy_text (), copy_data (), copy_sym (), copy_mod (), copy_sel ();
void write_objc_bind_sym (), write_global_syms (), write_shlib_syms ();
void write_objc_modDesc_sym ();

void
write_header ()
{
  if (strip_symbols == STRIP_ALL)
    nsyms = 0;
  else
    {
      nsyms = defined_global_sym_count + undefined_global_sym_count;
      if (discard_locals == DISCARD_L)
	nsyms += non_L_local_sym_count;
      else if (discard_locals == DISCARD_NONE)
	nsyms += local_sym_count;
    }

  if (strip_symbols == STRIP_NONE)
    nsyms += debugger_sym_count;

  if (!relocatable_output)
    {
      text_reloc_size = 0;
      data_reloc_size = 0;
    }

#ifdef MACH_O
  if (magic == MH_MAGIC)
    {
      int i, j, rel_offset;

      if (filetype == MH_OBJECT || filetype == MH_PRELOAD)
	rel_offset = mach_headers_size;
      else
	rel_offset = 0;

      mhp->magic = MH_MAGIC;
      mhp->cputype = MACHINE_CPU_TYPE;
      mhp->cpusubtype = MACHINE_CPU_SUBTYPE;
      if (filetype == MH_OBJECT)
	{
#ifdef NO_SYMSEGS
	  mhp->ncmds = 2;
#else
	  mhp->ncmds = 3;
#endif
	  mhp->sizeofcmds = sizeof(struct segment_command) +
			    6 * sizeof(struct section) +
			    sizeof(struct symtab_command)
#ifdef NO_SYMSEGS
			    ;
#else
			    + sizeof(struct symseg_command);
#endif
	}
      else
	{
#ifdef NO_SYMSEGS
	  mhp->ncmds = 4 + n_seg_create + n_fvmlibs;
#else
	  mhp->ncmds = 5 + n_seg_create + n_fvmlibs;
#endif
	  mhp->sizeofcmds = 3 * sizeof(struct segment_command) +
			    6 * sizeof(struct section) +
			    sizeof(struct symtab_command)
#ifdef NO_SYMSEGS
			    ;
#else
			    + sizeof(struct symseg_command);
#endif
	  if (pagezero_segment)
	   {
	      mhp->ncmds++;
	      mhp->sizeofcmds += sizeof(struct segment_command);
	   }
	  if (linkedit_segment)
	   {
	      mhp->ncmds++;
	      mhp->sizeofcmds += sizeof(struct segment_command);
	   }
	}

      for (i = 0; i < n_seg_create ; i++)
	mhp->sizeofcmds += seg_create[i].sg.cmdsize;
      if (ident.strings_size != 0){
	mhp->sizeofcmds += ident.lc_ident.cmdsize;
        mhp->ncmds++;
      }
      if (shlib_create)
	{
	  if (n_fvmlibs == 0)
	    fatal ("no shared library information symbol found");
	  if (n_fvmlibs > 1)
	    {
	      error ("multiple shared library information symbols found:");
	      for(i = 0; i < n_fvmlibs; i++)
		fprintf (stderr, " %s\n", fvmlibs[i].shlib.name);
	      fatal ("must only be one of these symbols");
	    }
	  mhp->filetype = MH_FVMLIB;
	}
      else
	{
	  mhp->filetype = filetype;
	  if (filetype != MH_OBJECT)
	    {
	      mhp->sizeofcmds += sizeof(struct thread_aout);
	      mhp->ncmds++;
	    }
	}
      for(i = 0; i < n_fvmlibs; i++)
	mhp->sizeofcmds += fvmlibs[i].lc_fvmlib.cmdsize;

      /* mhp->flags filled in with MH_NOUNDEFS or 0 in list_undefined_symbols */

      rel_mach.rel_seg.cmd = LC_SEGMENT;
      rel_mach.rel_seg.cmdsize = sizeof(struct segment_command) +
				 6 * sizeof(struct section);
      rel_mach.rel_seg.vmaddr = text_start;
      rel_mach.rel_seg.vmsize = text_size + data_size + bss_size +
				sym_size + mod_size + sel_size;
      rel_mach.rel_seg.fileoff = rel_offset;
      rel_mach.rel_seg.filesize = text_size + data_size +
				  sym_size + mod_size + sel_size;
      rel_mach.rel_seg.maxprot = VM_PROT_READ | VM_PROT_WRITE |
				 VM_PROT_EXECUTE;
      rel_mach.rel_seg.initprot = VM_PROT_READ | VM_PROT_WRITE |
				  VM_PROT_EXECUTE;
      rel_mach.rel_seg.nsects = 6;
      rel_mach.rel_seg.flags = 0;

      exec_mach.text_seg.cmd = LC_SEGMENT;
      exec_mach.text_seg.cmdsize = sizeof(struct segment_command) +
				 sizeof(struct section);
      strcpy(exec_mach.text_seg.segname, SEG_TEXT);
      exec_mach.text_seg.vmaddr = text_start;
      exec_mach.text_seg.vmsize = text_size;
      exec_mach.text_seg.fileoff = rel_offset;
      exec_mach.text_seg.filesize = text_size;
      exec_mach.text_seg.maxprot = VM_PROT_READ | VM_PROT_WRITE |
				   VM_PROT_EXECUTE;
      exec_mach.text_seg.initprot = VM_PROT_READ | VM_PROT_EXECUTE;
      exec_mach.text_seg.nsects = 1;
      exec_mach.text_seg.flags = 0;

      strcpy(tsp->sectname, SECT_TEXT);
      strcpy(tsp->segname, SEG_TEXT);
      if (filetype == MH_OBJECT || filetype == MH_PRELOAD)
	tsp->addr = text_start;
      else
	tsp->addr = text_start + mach_headers_size;
      tsp->offset = mach_headers_size;
      /*
       * This is filled in previously in digest_symbols() so it will have
       * the true size of the actual text without the padding:
       * 	tsp->size
       */
#if defined(I860)
      tsp->align = 5;	/* 256 bit alignment required for i860 instruction segs! */
#else
      tsp->align = 2;
#endif
      if (text_reloc_size != 0)
	{
	  tsp->reloff = rel_offset + text_size + data_size + objc_size +
			seg_create_size;
	  tsp->nreloc = text_reloc_size / sizeof (struct relocation_info);
	}
      else
	{
	  tsp->reloff = 0;
	  tsp->nreloc = 0;
	}
      tsp->flags = 0;
      tsp->reserved1 = 0;
      tsp->reserved2 = 0;

      exec_mach.data_seg.cmd = LC_SEGMENT;
      exec_mach.data_seg.cmdsize = sizeof(struct segment_command) +
				 2 * sizeof(struct section);
      strcpy(exec_mach.data_seg.segname, SEG_DATA);
      exec_mach.data_seg.vmaddr = data_start;
#if ! defined(I860)
      if (filetype == MH_PRELOAD)
	  exec_mach.data_seg.vmsize = data_size + bss_size;
      else
#endif
	  exec_mach.data_seg.vmsize = (data_size + bss_size + page_size - 1) &
				 (- page_size);
      exec_mach.data_seg.fileoff = rel_offset + text_size;
      exec_mach.data_seg.filesize = data_size;
      exec_mach.data_seg.maxprot = VM_PROT_READ | VM_PROT_WRITE |
				   VM_PROT_EXECUTE;
      exec_mach.data_seg.initprot =VM_PROT_READ | VM_PROT_WRITE |
				   VM_PROT_EXECUTE;
      exec_mach.data_seg.nsects = 2;
      exec_mach.data_seg.flags = 0;

      strcpy(dsp->sectname, SECT_DATA);
      strcpy(dsp->segname, SEG_DATA);
      dsp->addr = data_start;
      /*
       * This is filled in previously in digest_symbols() so it will have
       * the true size of the actual data without the padding:
       *	dsp->size
       */
      dsp->offset = rel_offset + text_size;
#if defined(I860)
      dsp->align = 4;	/* 128 bit alignment needed */
#else
      dsp->align = 2;
#endif
      if (data_reloc_size != 0)
	{
	  dsp->reloff = rel_offset + text_size + data_size + objc_size +
			seg_create_size + text_reloc_size;
	  dsp->nreloc = data_reloc_size / sizeof (struct relocation_info);
	}
      else
	{
	  dsp->reloff = 0;
	  dsp->nreloc = 0;
	}
      dsp->flags = 0;
      dsp->reserved1 = 0;
      dsp->reserved2 = 0;

      strcpy(bsp->sectname, SECT_BSS);
      strcpy(bsp->segname, SEG_DATA);
      bsp->addr = data_start + dsp->size;
      /*
       * This is filled in previously in digest_symbols() so it will have
       * the true size of the actual bss offset without the padding and moving
       * bss into the data area:
       *	bsp->size
       */
      bsp->offset = 0;
#if defined(I860)
      bsp->align = 4;	/* 128 bit alignment required by i860 */
#else
      bsp->align = 2;
#endif
      bsp->reloff = 0;
      bsp->nreloc = 0;
      bsp->flags = S_ZEROFILL;
      bsp->reserved1 = 0;
      bsp->reserved2 = 0;

      exec_mach.objc_seg.cmd = LC_SEGMENT;
      exec_mach.objc_seg.cmdsize = sizeof(struct segment_command) +
				   3 * sizeof(struct section);
      strcpy(exec_mach.objc_seg.segname, SEG_OBJC);
      exec_mach.objc_seg.vmaddr = objc_start;
      exec_mach.objc_seg.vmsize = objc_size;
      exec_mach.objc_seg.fileoff = rel_offset + text_size + data_size;
      exec_mach.objc_seg.filesize = objc_size;
      exec_mach.objc_seg.maxprot = VM_PROT_READ | VM_PROT_WRITE |
				   VM_PROT_EXECUTE;
      exec_mach.objc_seg.initprot =VM_PROT_READ | VM_PROT_WRITE |
				   VM_PROT_EXECUTE;
      exec_mach.objc_seg.nsects = 3;
      exec_mach.objc_seg.flags = 0;

      strcpy(symsp->sectname, SECT_OBJC_SYMBOLS);
      strcpy(symsp->segname, SEG_OBJC);
      symsp->addr = objc_start;
      symsp->size = sym_size;
      symsp->offset = rel_offset + text_size + data_size;
      symsp->align = 2;
      if (sym_reloc_size != 0)
	{
	  symsp->reloff = rel_offset + text_size + data_size + objc_size +
			  seg_create_size + text_reloc_size + data_reloc_size;
	  symsp->nreloc = sym_reloc_size / sizeof (struct relocation_info);
	}
      else
	{
	  symsp->reloff = 0;
	  symsp->nreloc = 0;
	}
      symsp->flags = 0;
      symsp->reserved1 = 0;
      symsp->reserved2 = 0;

      strcpy(modsp->sectname, SECT_OBJC_MODULES);
      strcpy(modsp->segname, SEG_OBJC);
      modsp->addr = objc_start + sym_size;
      modsp->size = mod_size;
      modsp->offset = rel_offset + text_size + data_size + sym_size;
      modsp->align = 2;
      if (mod_reloc_size != 0)
	{
	  modsp->reloff = rel_offset + text_size + data_size + objc_size +
			  seg_create_size + text_reloc_size + data_reloc_size +
			  sym_reloc_size;
	  modsp->nreloc = mod_reloc_size / sizeof (struct relocation_info);
	}
      else
	{
	  modsp->reloff = 0;
	  modsp->nreloc = 0;
	}
      modsp->flags = 0;
      modsp->reserved1 = 0;
      modsp->reserved2 = 0;

      strcpy(selsp->sectname, SECT_OBJC_STRINGS);
      strcpy(selsp->segname, SEG_OBJC);
      selsp->addr = objc_start + sym_size + mod_size;
      selsp->size = sel_size;
      selsp->offset = rel_offset + text_size + data_size + sym_size + mod_size;
      selsp->align = 2;
      if (sel_reloc_size != 0)
	{
	  selsp->reloff = rel_offset + text_size + data_size + objc_size +
			  seg_create_size + text_reloc_size + data_reloc_size +
			  sym_reloc_size + mod_reloc_size;
	  selsp->nreloc = sel_reloc_size / sizeof (struct relocation_info);
	}
      else
	{
	  selsp->reloff = 0;
	  selsp->nreloc = 0;
	}
      selsp->flags = S_CSTRING_LITERALS;
      selsp->reserved1 = 0;
      selsp->reserved2 = 0;

      stp->cmd = LC_SYMTAB;
      stp->cmdsize = sizeof (struct symtab_command);
      if (nsyms != 0)
        {
	  stp->symoff = rel_offset + text_size + data_size + objc_size +
			seg_create_size + text_reloc_size + data_reloc_size +
			sym_reloc_size + mod_reloc_size + sel_reloc_size;
	  stp->nsyms = nsyms;
	  stp->stroff = stp->symoff + nsyms * sizeof (struct nlist);
	  stp->strsize = 0; /* filled in later */
        }
      else
	{
	  stp->symoff = 0;
	  stp->nsyms = 0;
	  stp->stroff = 0;
	  stp->strsize = 0;
	}
#ifndef NO_SYMSEGS
      ssp->cmd = LC_SYMSEG;
      ssp->cmdsize = sizeof (struct symseg_command);;
      ssp->offset = 0; /* filled in later */
      ssp->size = 0; /* filled in later */
#endif

      thread.ut.cmd = LC_UNIXTHREAD;
#if defined(I860)
      thread.ut.cmdsize = sizeof (struct thread_command) +
			  2 * sizeof(unsigned long) +
			  sizeof(struct NextDimension_thread_state_regs);
      thread.flavor = NextDimension_THREAD_STATE_REGS;
      thread.count = NextDimension_THREAD_STATE_REGS_COUNT;
#else
      thread.ut.cmdsize = sizeof (struct thread_command) +
			  2 * sizeof(unsigned long) +
			  sizeof(struct NextDimension_thread_state_regs);
      thread.flavor = NeXT_THREAD_STATE_REGS;
      thread.count = NeXT_THREAD_STATE_REGS_COUNT;
#endif
      thread.cpu.pc = (entry_symbol ? entry_symbol->value
			          : text_start + entry_offset);

      if (pagezero_segment)
	{
	  pagezero_segment->cmd = LC_SEGMENT;
	  pagezero_segment->cmdsize = sizeof(struct segment_command);
	  strcpy(pagezero_segment->segname, SEG_PAGEZERO);
	  pagezero_segment->vmaddr = 0;
	  pagezero_segment->vmsize = page_size;
	  pagezero_segment->fileoff = 0;
	  pagezero_segment->filesize = 0;
	  pagezero_segment->maxprot = VM_PROT_NONE;
	  pagezero_segment->initprot = VM_PROT_NONE;
	  pagezero_segment->nsects = 0;
	  pagezero_segment->flags = 0;
	}

      if(filetype != MH_PRELOAD)
	for (i = 0; i < n_seg_create ; i++)
	  seg_create[i].sg.flags |= SG_NORELOC;

      if (linkedit_segment)
	{
	  linkedit_segment->cmd = LC_SEGMENT;
	  linkedit_segment->cmdsize = sizeof(struct segment_command);
	  strcpy(linkedit_segment->segname, SEG_LINKEDIT);
	  /* linkedit_segment->vmaddr filled in previously */
	  linkedit_segment->vmsize = 0; /* filled in later */
	  linkedit_segment->fileoff = rel_offset + text_size + data_size +
				      objc_size + seg_create_size;
	  linkedit_segment->filesize = 0; /* filled in later */
      	  linkedit_segment->maxprot = VM_PROT_READ | VM_PROT_WRITE |
				      VM_PROT_EXECUTE;
      	  linkedit_segment->initprot =VM_PROT_READ | VM_PROT_WRITE |
				      VM_PROT_EXECUTE;
	  linkedit_segment->nsects = 0;
	  linkedit_segment->flags = SG_NORELOC;
	}
      /*
       * Since all the values can't be set here the headers are written last
       * so here we just seek to get the file pointer in the right place.
       */
      lseek (outdesc, mach_headers_size, L_SET);
    }
  else
#endif MACH_O
    {
#ifdef sun
      outheader.a_machtype = M_68020;
#endif
#if defined(NeXT) && !defined(I860)
      outheader.a_machtype = 2;
#endif
      outheader.a_magic = magic;
      outheader.a_text = text_size;
      outheader.a_data = data_size;
      outheader.a_bss = bss_size;
      outheader.a_entry = (entry_symbol ? entry_symbol->value
			   : text_start + entry_offset);
      outheader.a_syms = nsyms * sizeof (struct nlist);
      outheader.a_trsize = text_reloc_size;
      outheader.a_drsize = data_reloc_size;

      mywrite (&outheader, sizeof (struct exec), 1, outdesc);

      /* Output whatever padding is required in the executable file
	 between the header and the start of the text.  */
      padfile (N_TXTOFF (outheader) - sizeof outheader, outdesc);
    }
}


/* Relocate the text segment of each input file
   and write to the output file.  */

void
write_text ()
{
  if (trace_files)
    fprintf (stderr, "Copying and relocating text:\n\n");

  each_full_file (copy_text);
  file_close ();

  if (trace_files)
    fprintf (stderr, "\n");

  padfile (text_pad, outdesc);
}

/* Read the text segment contents of ENTRY, relocate them,
   and write the result to the output file.
   If `-r', save the text relocation for later reuse.  */

void
copy_text (entry)
     struct file_entry *entry;
{
  register char *bytes;
  register int desc;
  register struct relocation_info *reloc;

  if (trace_files)
    prline_file_name (entry, stderr);

  if (entry->header.a_text == 0)
    return;

  desc = file_open (entry);

  /* Allocate space for the file's text section and text-relocation.  */

  bytes = (char *) xmalloc (entry->header.a_text);
  reloc = (struct relocation_info *) xmalloc (entry->header.a_trsize);

  /* Read those two sections into core.  */

  lseek (desc, entry->starting_offset + entry->text_offset, 0);
  if (entry->header.a_text != read (desc, bytes, entry->header.a_text))
    fatal_with_file (entry, "Premature eof in text section of: ");

  lseek (desc, entry->starting_offset + entry->text_reloc_offset, 0);
  if (entry->header.a_trsize != read (desc, reloc, entry->header.a_trsize))
    fatal_with_file (entry, "Premature eof in text relocation of: ");

  /* Relocate the text according to the text relocation.  */

  perform_relocation (bytes, entry->text_start_address, 0, entry->header.a_text,
		      reloc, entry->header.a_trsize, entry);

  /* Write the relocated text to the output file.  */

  mywrite (bytes, 1, entry->header.a_text, outdesc);

  /* If `-r', record the text relocation permanently
     so the combined relocation can be written later.  */

  if (relocatable_output)
    entry->textrel = reloc;
  else
      free (reloc);

  free (bytes);
}

/* flags for copy_data's argument */
#define ALL_DATA	0
#define GLOBAL_DATA	1
#define STATIC_DATA	2

/* Relocate the data segment of each input file
   and write to the output file.  */

void
write_data ()
{
  if (trace_files)
    fprintf (stderr, "Copying and relocating data:\n\n");

#ifdef SHLIB
  if (shlib_create)
    {
      if (do_print_inits == 0)
        do_print_inits = 1;
      each_full_file (copy_data, GLOBAL_DATA);
      if (do_print_inits == 1)
        do_print_inits = -1;
      each_full_file (copy_data, STATIC_DATA);
      if (do_print_inits == -1)
        do_print_inits = 0;
    }
  else
#endif SHLIB
      each_full_file (copy_data, ALL_DATA);
  file_close ();

  if (trace_files)
    fprintf (stderr, "\n");

  /* Output the objective C module descriptor table. */
  write_objc_bind_sym ();
  write_objc_modDesc_sym ();
  /* Output global syms to data segment if requested */
#ifdef SHLIB
  /* Output the shared library information and initialization tables */
  write_shlib_syms ();
#endif SHLIB
  write_global_syms ();

  padfile (data_pad, outdesc);
}


/* Read the data segment contents of ENTRY, relocate them,
   and write the result to the output file.
   If `-r', save the data relocation for later reuse.
   See comments in `copy_text'.  */

void
copy_data (entry, flag)
     struct file_entry *entry;
     int flag; 
{
  register struct relocation_info *reloc;
  register char *bytes;
  register int desc;

  if (trace_files)
    prline_file_name (entry, stderr);

  if (entry->header.a_data == 0)
    return;

  desc = file_open (entry);

  bytes = (char *) xmalloc (entry->header.a_data);
  reloc = (struct relocation_info *) xmalloc (entry->header.a_drsize);

  lseek (desc, entry->starting_offset + entry->data_offset, 0);
  if (entry->header.a_data != read (desc, bytes, entry->header.a_data))
    fatal_with_file (entry, "Premature eof in data section of: ");

  lseek (desc, entry->starting_offset + entry->data_reloc_offset,0);
  if (entry->header.a_drsize != read (desc, reloc, entry->header.a_drsize))
    fatal_with_file (entry, "Premature eof in data relocation of: ");

  perform_relocation (bytes, entry->data_start_address, entry->header.a_text,
		      entry->header.a_data, reloc, entry->header.a_drsize, entry);

#ifdef SHLIB
  /* For shared library output the global data and static data are written
     separately.  This involves write_data() to call copy_data() twice for
     each data segment in all the input file.  This is again is a KLUGE. */
  if (shlib_create)
    {
      if (flag == GLOBAL_DATA)
	mywrite(bytes + entry->first_global_data_address - entry->header.a_text,
		1, (entry->header.a_text + entry->header.a_data) -
		entry->first_global_data_address, outdesc);
      else
	mywrite(bytes, 1, entry->first_global_data_address -
	        entry->header.a_text, outdesc);
    }
  else
#endif SHLIB
    mywrite (bytes, 1, entry->header.a_data, outdesc);

  if (relocatable_output)
    entry->datarel = reloc;
  else
    free (reloc);

  free (bytes);
}

/* Relocate the objective-C segment of each input file
   and write to the output file.  */

void
write_objc ()
{
  if (trace_files)
    fprintf (stderr, "Copying and relocating objective-C segment:\n\n");

  if(objc_size == 0)
    return;

  each_full_file (copy_sym);
  file_close ();
  each_full_file (copy_mod);
  file_close ();
  write_sel ();

  if (trace_files)
    fprintf (stderr, "\n");

  padfile (objc_size - (sym_size + mod_size + sel_size), outdesc);
}

/* Read the objective-C symbol section contents of ENTRY, relocate them,
   and write the result to the output file.
   If `-r', save the text relocation for later reuse.  */

void
copy_sym (entry)
     struct file_entry *entry;
{
  register char *bytes;
  register int desc;
  register struct relocation_info *reloc;

  if (trace_files)
    prline_file_name (entry, stderr);

  if(entry->sym_size == 0)
    return;

  desc = file_open (entry);

  /* Allocate space for the section and relocation.  */

  bytes = (char *) xmalloc (entry->sym_size);
  reloc = (struct relocation_info *) xmalloc (entry->sym_reloc_size);

  /* Read those two sections into core.  */

  lseek (desc, entry->starting_offset + entry->sym_offset, 0);
  if (entry->sym_size != read (desc, bytes, entry->sym_size))
    fatal_with_file(entry, "Premature eof in %s section of: ",
		    SECT_OBJC_SYMBOLS);

  lseek (desc, entry->starting_offset + entry->sym_reloc_offset, 0);
  if (entry->sym_reloc_size != read (desc, reloc, entry->sym_reloc_size))
    fatal_with_file (entry, "Premature eof in %s relocation of: ",
		     SECT_OBJC_SYMBOLS);

  /* Relocate the text according to the text relocation.  */

  perform_relocation (bytes, entry->sym_start_address, entry->sym_addr,
		      entry->sym_size, reloc, entry->sym_reloc_size, entry);

  /* Write the relocated objective-C symbols to the output file.  */

  mywrite (bytes, 1, entry->sym_size, outdesc);

  /* If `-r', record the text relocation permanently
     so the combined relocation can be written later.  */

  if (relocatable_output)
    entry->symrel = reloc;
  else
      free (reloc);

  free (bytes);
}

/* Read the objective-C module info section contents of ENTRY, relocate them,
   and write the result to the output file.
   If `-r', save the text relocation for later reuse.  */

void
copy_mod (entry)
     struct file_entry *entry;
{
  register char *bytes;
  register int desc;
  register struct relocation_info *reloc;

  if (trace_files)
    prline_file_name (entry, stderr);

  if(entry->mod_size == 0)
    return;

  desc = file_open (entry);

  /* Allocate space for the section and relocation.  */

  bytes = (char *) xmalloc (entry->mod_size);
  reloc = (struct relocation_info *) xmalloc (entry->mod_reloc_size);

  /* Read those two sections into core.  */

  lseek (desc, entry->starting_offset + entry->mod_offset, 0);
  if (entry->mod_size != read (desc, bytes, entry->mod_size))
    fatal_with_file(entry, "Premature eof in %s section of: ",
		    SECT_OBJC_SYMBOLS);

  lseek (desc, entry->starting_offset + entry->mod_reloc_offset, 0);
  if (entry->mod_reloc_size != read (desc, reloc, entry->mod_reloc_size))
    fatal_with_file (entry, "Premature eof in %s relocation of: ",
		     SECT_OBJC_SYMBOLS);

  /* Relocate the text according to the text relocation.  */

  perform_relocation (bytes, entry->mod_start_address, entry->mod_addr,
		      entry->mod_size, reloc, entry->mod_reloc_size, entry);

  /* Write the relocated objective-C module info to the output file.  */

  mywrite (bytes, 1, entry->mod_size, outdesc);

  /* If `-r', record the text relocation permanently
     so the combined relocation can be written later.  */

  if (relocatable_output)
    entry->modrel = reloc;
  else
      free (reloc);

  free (bytes);
}

write_sel ()
{
  struct sel_block *p;
  for (p = sel_blocks; p ; p = p->next)
    {
/*
printf("In write_sel writing %d bytes (%s)\n", p->used, p->sels);
*/
      mywrite (p->sels, 1, p->used, outdesc);
    }
}

#ifdef 0
/* Read the objective-C string section contents of ENTRY, relocate them,
   and write the result to the output file.
   If `-r', save the text relocation for later reuse.  */

void
copy_sel (entry)
     struct file_entry *entry;
{
  register char *bytes;
  register int desc;
  register struct relocation_info *reloc;

  if (trace_files)
    prline_file_name (entry, stderr);

  if(entry->sel_size == 0)
    return;

  desc = file_open (entry);

  /* Allocate space for the section and relocation.  */

  bytes = (char *) xmalloc (entry->sel_size);
  reloc = (struct relocation_info *) xmalloc (entry->sel_reloc_size);

  /* Read those two sections into core.  */

  lseek (desc, entry->starting_offset + entry->sel_offset, 0);
  if (entry->sel_size != read (desc, bytes, entry->sel_size))
    fatal_with_file(entry, "Premature eof in %s section of: ",
		    SECT_OBJC_SYMBOLS);

  lseek (desc, entry->starting_offset + entry->sel_reloc_offset, 0);
  if (entry->sel_reloc_size != read (desc, reloc, entry->sel_reloc_size))
    fatal_with_file (entry, "Premature eof in %s relocation of: ",
		     SECT_OBJC_SYMBOLS);

  /* Relocate the text according to the text relocation.  */

  perform_relocation (bytes, 0, entry->sel_addr, entry->sel_size,
		      reloc, entry->sel_reloc_size, entry);

  /* Write the relocated objective-C strings to the output file.  */

  mywrite (bytes, 1, entry->sel_size, outdesc);

  /* If `-r', record the text relocation permanently
     so the combined relocation can be written later.  */

  if (relocatable_output)
    entry->selrel = reloc;
  else
      free (reloc);

  free (bytes);
}
#endif


/* Faked up values for r_symbolnum that can collide with real ones because
   r_symbolnum is 24 bits wide.  */
#define N_OBJC_SYM	0x01000000
#define N_OBJC_MOD	0x02000000
#define N_OBJC_SEL	0x03000000

/* Relocate ENTRY's text or data section contents.
   DATA is the address of the contents, in core.
   DATA_SIZE is the length of the contents.
   NEW_BASE is the address of the contents in the output file.
   OLD_BASE is the address of the contents in the input file.
   RELOC_INFO is the address of the relocation info, in core.
   RELOC_SIZE is its length in bytes.  */

void
perform_relocation (data, new_base, old_base, data_size, reloc_info, reloc_size, entry)
     char *data;
     int new_base, old_base;
     struct relocation_info *reloc_info;
     struct file_entry *entry;
     int data_size;
     int reloc_size;
{
  register struct relocation_info *p = reloc_info;
  struct relocation_info *end
    = reloc_info + reloc_size / sizeof (struct relocation_info);
  int pc_relocation = new_base - old_base;
  int text_relocation = entry->text_start_address;
  int data_relocation = entry->data_start_address - entry->header.a_text;
  int bss_relocation = entry->bss_start_address - entry->bss_addr;
  int sym_relocation = entry->sym_start_address - entry->sym_addr;
  int mod_relocation = entry->mod_start_address - entry->mod_addr;
  int sel_start = objc_start + sym_size + mod_size;
  int new_pc, old_pc;
	
#ifdef SHLIB
  int reloc_item, global_data_relocation, static_data_relocation;

  if (shlib_create)
    {
      if(do_print_inits >= 0)
	print_init_setup(entry);
      global_data_relocation = entry->global_data_start_address -
			       entry->first_global_data_address;
      static_data_relocation = entry->static_data_start_address -
			       entry->header.a_text;
    }
#endif SHLIB
  for (; p < end; p++)
    {
      register int relocation = 0;
      register int addr = p->r_address;
      register int symbolnum = p->r_symbolnum;
      register int length = p->r_length;
      if (addr >= data_size)
	fatal_with_file (entry, "Relocation address out of range (0x%x > 0x%x) in: ",
		addr, data_size );
      if (p->r_extern)
	{
	  int symindex = symbolnum * sizeof (struct nlist);
	  struct nlist *nlp;
	  symbol *sp;
	  if (symindex >= entry->header.a_syms)
	    fatal_with_file (entry, "Relocation symbolnum out of range in: ");
	  nlp = (struct nlist *) (((char *)entry->symbols) + symindex);
	  if (((nlp->n_type & N_EXT)) == 0)
	    fatal_with_file (entry, "External relocation entry for "
			     "non-external symbol in: ");
	  sp = (symbol *)(nlp->n_un.n_name);

	  /* Resolve indirection */
	  if ((sp->defined & ~N_EXT) == N_INDR)
	    sp = (symbol *) sp->value;

	  /* If the symbol is undefined, leave it at zero.  */
	  if (! sp->defined)
	    {
	      relocation = 0;
#ifdef SHLIB
	      if (shlib_create)
		if(do_print_inits == 1) /* data section */
		  print_init(entry->filename, addr + entry->header.a_text,
			     sp->name);
		else if(do_print_inits == 0) /* text section */
		  printf("relocation entry for undefined symbol %s at text "
			 "offset 0x%x in %s\n",sp->name, addr, entry->filename);
/*
		printf("#init %s 0x%x\t%s\n", entry->filename, addr, sp->name);
*/
#endif SHLIB
	    }
	  else
	    relocation = sp->value;
	}
      else
	{
	  /* Local relocation entries, r_extern == 0.  */
	  if (entry->mach)
	    {
	      switch (symbolnum)
		{
		case 1:
		  symbolnum = N_TEXT;
		  break;
		case 2:
		  symbolnum = N_DATA;
		  break;
		case 3:
		  symbolnum = N_BSS;
		  break;
		case 4:
		  symbolnum = N_OBJC_SYM;
		  break;
		case 5:
		  symbolnum = N_OBJC_MOD;
		  break;
		case 6:
		  symbolnum = N_OBJC_SEL;
		  break;
		case R_ABS:
		  symbolnum = N_ABS;
		  break;
		default:
#if defined(I860)
		  if ( p->r_type == RELOC_PAIR )
		      continue; /* Entry used only with RELOC_HIGH or RELOC_HIGHADJ */
#endif
		  fatal_with_file (entry, "Nonexternal relocation r_symbolnum "
				  "(%d) invalid for relocation entry (%d) in: ",
				  symbolnum, p - reloc_info);
		}
	    }

	  switch (symbolnum)
	    {
	    case N_TEXT:
	    case N_TEXT | N_EXT:
	      relocation = text_relocation;
	      break;

	    case N_DATA:
	    case N_DATA | N_EXT:
	      /* A word that points to beginning of the the data section
		 initially contains not 0 but rather the "address" of that
		 section in the input file, which is the length of the file's
		 text.  */
#ifdef SHLIB
	      if (shlib_create)
		{
		  switch (length)
		    {
		    case 0:
		      reloc_item = *(char *) (data + addr);
		      break;
		    case 1:
		      reloc_item = *(short *) (data + addr);
		      break;
		    case 2:
		      reloc_item = *(int *) (data + addr);
		      break;
		    default:
		      fatal_with_file (entry, "invalid relocation field length "
				       "r_length (%d) for relocation entry "
				       "(%d) in: ", length, p - reloc_info);
		    }
		  /*
		   * This can NOT be done right because of symbol plus offset
		   * relocation.  If the reloc_item is not in the same global
		   * or static data section this will be wrong.  It must be
		   * done at the assembler when the symbol value is know before
		   * the offset is added.  For now it is a serious bug that can
		   * be fixed by having no global data and static data in the
		   * same file.
		   */
		  if (reloc_item < entry->first_global_data_address ||
      		      entry->first_global_data_address ==
		      entry->header.a_text + entry->header.a_data)
		    relocation = static_data_relocation;
		  else
		    relocation = global_data_relocation;
		}
	      else
#endif SHLIB
		relocation = data_relocation;
	      break;

	    case N_BSS:
	    case N_BSS | N_EXT:
	      /* Similarly, an input word pointing to the beginning of the bss
		 initially contains the length of text plus data of the file.  */
	      relocation = bss_relocation;
	      break;

	    case N_OBJC_SYM:
	      relocation = sym_relocation;
	      break;

	    case N_OBJC_MOD:
	      relocation = mod_relocation;
	      break;

	    case N_OBJC_SEL:
	      {
		if (p->r_pcrel)
		  {
		    new_pc = new_base + addr;
		    old_pc = old_base + addr;
		  }
		else
		  {
		    new_pc = 0;
		    old_pc = 0;
		  }

		switch (length)
		  {
		  case 0:
		    *(char *) (data + addr) = sel_start +
			map_selector_offset (entry, *(char *) (data + addr) -
					     entry->sel_addr + old_pc) - new_pc;
		    break;

		  case 1:
		    *(short *) (data + addr) = sel_start +
			map_selector_offset (entry, *(short *) (data + addr) -
					     entry->sel_addr + old_pc) - new_pc;
		    break;

		  case 2:
		    *(int *) (data + addr) = sel_start +
			map_selector_offset (entry, *(int *) (data + addr) -
					     entry->sel_addr + old_pc) - new_pc;
		    break;

		  default:
		    fatal_with_file (entry, "invalid relocation field length "
				     "r_length (%d) for relocation entry "
				     "(%d) in: ", length, p - reloc_info);
		  }
		continue;
	      }

	    case N_ABS:
	      continue;

	    default:
	      fatal_with_file (entry, "Nonexternal relocation r_symbolnum "
			      "(%d) invalid for relocation entry (%d) in: ",
			      symbolnum, p - reloc_info);
	    }
	}
      if (p->r_pcrel)
	relocation -= pc_relocation;
#if defined(I860)
      if ( p->r_type == RELOC_VANILLA )
      {
	  switch (length)
	    {
	    case 0:
	      *(char *) (data + addr) += relocation;
	      break;
    
	    case 1:
	      *(short *) (data + addr) += relocation;
	      break;
    
	    case 2:
	      *(int *) (data + addr) += relocation;
	      break;
    
	    default:
	      fatal_with_file (entry, "invalid relocation field length "
			       "r_length (%d) for relocation entry "
			       "(%d) in: ", length, p - reloc_info);
	    }
      }
      else	/* Funky relocatable text bitfields */
      {
      	   unsigned char *buf;
	   long opcode;
	   long immed;
	   struct relocation_info *rpair;
	   
	    buf = (unsigned char *)(data+addr);
#ifdef BYTE_SWAP
	    /*
	     * Instructions are little-endian.
	     * Repack to native order to preserve sanity.
	     */
	    opcode = (buf[3] << 24) | (buf[2] << 16) | (buf[1] << 8) | buf[0];
#else
	    opcode = (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];
#endif
      	    switch( p->r_type )
	    {
	    case RELOC_HIGH:
	        rpair = p + 1;
		if ( rpair->r_type != RELOC_PAIR )
	            fatal_with_file (entry, "Relocation RELOC_HIGH entry (%d) "
			          "missing associated RELOC_PAIR item in: ",
			          p - reloc_info);
		immed = ((opcode & 0xFFFF) << 16) | (rpair->r_symbolnum & 0xFFFF);
		immed += relocation;
		opcode &= ~0xFFFF;
		opcode |= ((immed >> 16) & 0xFFFF);
		/* Update relocation pair item. */
		rpair->r_symbolnum = (immed & 0xFFFF) | 0x7F0000;
		/* Note: the high bits are set to avoid confusion with storage class */
		break;
		
	    case RELOC_LOW0:
		immed = (opcode & 0xFFFF);
		if ( immed & 0x8000 )	/* sign extend as appropriate. */
			immed |= 0xFFFF0000;
		immed += relocation;
		opcode &= ~0xFFFF;
		opcode |= (immed & 0xFFFF);
		break;
		
	    case RELOC_LOW1:
		immed = (opcode & 0xFFFE);
		if ( immed & 0x8000 )	/* sign extend as appropriate. */
			immed |= 0xFFFF0000;
		immed += relocation;
		opcode &= 0xFFFF0001;
		opcode |= (immed & 0xFFFE);
		break;
		
	    case RELOC_LOW2:
		immed = (opcode & 0xFFFC);
		if ( immed & 0x8000 )	/* sign extend as appropriate. */
			immed |= 0xFFFF0000;
		immed += relocation;
		opcode &= 0xFFFF0003;
		opcode |= (immed & 0xFFFC);
		break;
		
	    case RELOC_LOW3:
		immed = (opcode & 0xFFF8);
		if ( immed & 0x8000 )	/* sign extend as appropriate. */
			immed |= 0xFFFF0000;
		immed += relocation;
		opcode &= 0xFFFF0007;
		opcode |= (immed & 0xFFF8);
		break;
		
	    case RELOC_LOW4:
		immed = (opcode &0xFFF0);
		if ( immed & 0x8000 )	/* sign extend as appropriate. */
			immed |= 0xFFFF0000;
		immed += relocation;
		opcode &= 0xFFFF000F;
		opcode |= (immed & 0xFFF0);
		break;
		
	    case RELOC_SPLIT0:	/* 16 bit immed or branch insn */
	    	immed = ((opcode >> 5) & 0xF800) | (opcode & 0x7FF);
		if ( immed & 0x8000 )	/* sign extend as appropriate. */
			immed |= 0xFFFF0000;
		if ( p->r_pcrel )
			immed <<= 2;	/* word to byte address conversion */
		immed += relocation;
		if ( p->r_pcrel )
			immed >>= 2;	/* Back to word address */
		opcode &= 0xFFE0F800;
		opcode |= ((immed & 0xF800) << 5) | (immed & 0x7FF);
		break;
		
	    case RELOC_SPLIT1:
	    	immed = ((opcode >> 5) & 0xF800) | (opcode & 0x7FE);
		if ( immed & 0x8000 )	/* sign extend as appropriate. */
			immed |= 0xFFFF0000;
		immed += relocation;
		opcode &= 0xFFE0F801;
		opcode |= ((immed & 0xF800) << 5) | (immed & 0x7FE);
		break;
		
	    case RELOC_SPLIT2:
	    	immed = ((opcode >> 5) & 0xF800) | (opcode & 0x7FC);
		if ( immed & 0x8000 )	/* sign extend as appropriate. */
			immed |= 0xFFFF0000;
		immed += relocation;
		opcode &= 0xFFE0F803;
		opcode |= ((immed & 0xF800) << 5) | (immed & 0x7FC);
		break;
		
	    case RELOC_HIGHADJ:
	        rpair = p + 1;
		if ( rpair->r_type != RELOC_PAIR )
	            fatal_with_file (entry, "Relocation RELOC_HIGHADJ entry (%d) "
			          "missing associated RELOC_PAIR item in: ",
			          p - reloc_info);
		immed = ((opcode & 0xFFFF) << 16) | (rpair->r_symbolnum & 0xFFFF);
		immed += relocation;
		/* Update relocation pair item. */
		rpair->r_symbolnum = (immed & 0xFFFF) | 0x7F0000;
		/* Note: the high bits are set to avoid confusion with storage class */
		
		/*
		 * Don't do the adjustment if the output could go through
		 * the loader again.  Adjustment should be done once only, when
		 * preparing the final non-relocatable binary image.
		 */
		if ( (immed & 0x8000) && ! relocatable_output )
			immed = (immed >> 16) + 1;
		else
			immed = (immed >> 16);

		opcode &= ~0xFFFF;
		opcode |= (immed & 0xFFFF);
		break;
		
	    case RELOC_BRADDR:
	        immed = opcode & 0x03FFFFFF;
		if ( immed & 0x02000000 )	/* Sign extend as needed */
			immed |= 0xFC000000;
		if ( p->r_pcrel )
			immed <<= 2;
		immed += relocation;
		if ( p->r_pcrel )
			immed >>= 2;
		opcode &= 0xFC000000;
		opcode |= (immed & 0x03FFFFFF);
	    	break;
		
	    case NO_RELOC:
	    case RELOC_PAIR:
	    default:
	      fatal_with_file (entry, "invalid relocation field type "
			       "r_type (%d) for relocation entry "
			       "(%d) in: ", p->r_type, p - reloc_info);
	    
	    }
#ifdef BYTE_SWAP
	    /* Repack to i860 little-endian format.  */
	    buf[0] = opcode;
	    buf[1] = opcode >> 8;
	    buf[2] = opcode >> 16;
	    buf[3] = opcode >> 24;
#else
	    buf[3] = opcode;
	    buf[2] = opcode >> 8;
	    buf[1] = opcode >> 16;
	    buf[0] = opcode >> 24;
#endif
      }
#else	/* ! I860  (CISC style relocation stuff) */
      switch (length)
	{
	case 0:
	  *(char *) (data + addr) += relocation;
	  break;

	case 1:
	  *(short *) (data + addr) += relocation;
	  break;

	case 2:
	  *(int *) (data + addr) += relocation;
	  break;

	default:
	  fatal_with_file (entry, "invalid relocation field length "
			   "r_length (%d) for relocation entry "
			   "(%d) in: ", length, p - reloc_info);
	}
#endif
    }
#ifdef SHLIB
  if(do_print_inits >= 0)
    print_init_takedown();
#endif SHLIB
}


#ifdef SHLIB
/*
 * The stuff to print the shared library #init
 * section of the shared library specification file for undefined symbols
 * that have relocation entries.
 */
static struct nlist *sort_symbols = 0;
static char *sort_strings = 0;
static long nsort_symbols = 0;
static long print_init_filename = 0;

static long sym_compare();

print_init_setup (entry)
     struct file_entry *entry;
{
  struct nlist *temp_symbols;
  int desc, i;

  desc = file_open (entry);

  print_init_filename = 0;
  temp_symbols = (struct nlist *) xmalloc (entry->header.a_syms);
  sort_symbols = (struct nlist *) xmalloc (entry->header.a_syms);
  sort_strings = (char *) xmalloc (entry->string_size);

  lseek (desc, entry->starting_offset + entry->symbols_offset, 0);
  if (entry->header.a_syms != read (desc, temp_symbols, entry->header.a_syms))
    fatal_with_file (entry, "Premature end of file in symbols of: ");

  lseek (desc, entry->starting_offset + entry->strings_offset, 0);
  if (entry->string_size != read (desc, sort_strings, entry->string_size))
    fatal_with_file (entry, "Premature end of file in strings of ");

  nsort_symbols = 0;
  for (i = 0; i < entry->header.a_syms / sizeof(struct nlist); i++)
    {
      if (temp_symbols[i].n_type & N_EXT &&
          (temp_symbols[i].n_type & N_TYPE) != N_UNDF)
	{
	  sort_symbols[nsort_symbols] = temp_symbols[i];
	  if(sort_symbols[nsort_symbols].n_un.n_strx > 0 &&
	     sort_symbols[nsort_symbols].n_un.n_strx < entry->string_size)
	      sort_symbols[nsort_symbols].n_un.n_name =
		sort_strings + sort_symbols[nsort_symbols].n_un.n_strx;
	  else
	    fatal_with_file (entry, "Bad string table index (%d) for symbol "
			     "(%d) in: ",
	  		     sort_symbols[nsort_symbols].n_un.n_strx, i);
	  nsort_symbols++;
	}
    }
  qsort(sort_symbols, nsort_symbols, sizeof(struct nlist), sym_compare);
  free(temp_symbols);
}

static
long
sym_compare(sym1, sym2)
struct nlist *sym1, *sym2;
{
    return(sym1->n_value - sym2->n_value);
}

print_init_takedown ()
{
  if (print_init_filename)
    printf("\n");
  print_init_filename = 0;
  if (sort_symbols)
    free (sort_symbols);
  sort_symbols = (struct nlist *)0;
  if (sort_strings)
    free (sort_strings);
  sort_strings = (char *)0;
  nsort_symbols = 0;
}

print_init (filename, addr, symname)
    char *filename, *symname;
    long addr;
{
  int i, found;

  if (!print_init_filename)
    {
      printf ("#init %s\n", filename);
      print_init_filename = 1;
    }
  
  found = -1;
  for (i = 0; i < nsort_symbols; i++)
    {
      if (addr >= sort_symbols[i].n_value)
	found = i;
    }
  if (found != -1)
    {
      if (addr == sort_symbols[found].n_value)
	printf("\t%s\t\t%s\n", sort_symbols[found].n_un.n_name, symname);
      else
	printf("\t%s+%d\t\t%s\n", sort_symbols[found].n_un.n_name, 
      	       addr - sort_symbols[found].n_value, symname);
    }
  else
    printf("\t0x%x\t%s\n", addr, symname);
}
#endif SHLIB

#ifdef MACH_O
/* Write the contents of the files that create segments into the output file. */
void
write_seg_create ()
{
  register int i, j, max_buf_size, indesc;
  struct section_list *slp;
  char *buf;

  if (magic != MH_MAGIC)
    fatal ("-segcreate can only be used with -Mach (mach output)");
  if (filetype == MH_OBJECT)
    fatal ("-segcreate can't be used with relocatable output");

  if (trace_files)
    fprintf (stderr, "Writing segments created from files:\n\n");

  max_buf_size = 0;
  for ( i = 0; i < n_seg_create; i++)
    {
      slp = seg_create[i].slp;
      for (j = 0 ; j < seg_create[i].sg.nsects ; j++)
	{
	  if(max_buf_size < slp->filesize)
	    max_buf_size = slp->filesize;
	  slp = slp->next;
	}
    }
  buf = (char *) xmalloc (max_buf_size);
  for ( i = 0; i < n_seg_create; i++)
    {
      seg_create[i].sg.fileoff = lseek (outdesc, 0, L_INCR);
      slp = seg_create[i].slp;
      for (j = 0 ; j < seg_create[i].sg.nsects ; j++)
	{
	  if (trace_files)
	    fprintf (stderr, "Creating section %s in segment %s from file %s\n",
		     slp->s.sectname, slp->s.segname, slp->filename);
	  if ((indesc = open(slp->filename, O_RDONLY)) == -1)
	    perror_fatal ("Can't open file: %s to create segment with it's "
		          "contents", slp->filename);
	  if (read(indesc, buf, slp->filesize) !=
	      slp->filesize)
	    perror_fatal ("Can't read file: %s to create segment with it's "
		          "contents", slp->filename);
	  slp->s.offset = lseek (outdesc, 0, L_INCR);
	  if (write(outdesc, buf, slp->filesize) !=
		  slp->filesize)
	    perror_fatal ("Can't write contents of created segment to output "
			  "file: %s", output_filename);
	  padfile (slp->s.size - slp->filesize, outdesc);
	  close (indesc);
	  slp = slp->next;
	}
      padfile (seg_create[i].sg.vmsize - seg_create[i].sizeofsects, outdesc);
    }
    if (trace_files)
      fprintf (stderr, "\n");
    free (buf);
}
#endif MACH_O

/* Write the objective C module descriptor table (pre 1.0 style) */
void
write_objc_bind_sym ()
{
  register int i;
  struct bindlist *p;

  if (!objc_bind_sym.sp)
    return;

  p = objc_bind_sym.bindlist;
  for ( i = 0; i < objc_bind_sym.n_bindlist; i++)
    {
      if (p->link)
	mywrite (&(p->link->value), 1, sizeof(long), outdesc);
      else
	mywrite (&(p->link), 1, sizeof(long), outdesc);
      if (p->info)
	mywrite (&(p->info->value), 1, sizeof(long), outdesc);
      else
	mywrite (&(p->info), 1, sizeof(long), outdesc);
      p = p->next;
    }
}

/* Write the objective C module descriptor table (1.0 and later style) */
void
write_objc_modDesc_sym ()
{
  long i, absolutes, zero;
  struct symlist *p;

  if (!objc_modDesc_sym.sp)
    return;

  zero = 0;
  /*
   * First entry in the indirect table is the single table module descriptor
   * table that is written after the indirect table.
   */
  mywrite (&(objc_modDesc_sym.tab_value), 1, sizeof(long), outdesc);

  p = objc_modDesc_sym.tablelist;
  for ( i = 0; i < objc_modDesc_sym.n_tables; i++)
    {
      if (p->sp)
	mywrite (&(p->sp->value), 1, sizeof(long), outdesc);
      else
	mywrite (&zero, 1, sizeof(long), outdesc);
      p = p->next;
    }

  absolutes = 0;
  p = objc_modDesc_sym.desclist;
  for ( i = 0; i < objc_modDesc_sym.n_descs; i++)
    {
      if (p->sp)
	if (p->sp->defined != N_ABS | N_EXT)
	  mywrite (&(p->sp->value), 1, sizeof(long), outdesc);
	else
	  absolutes++;
      else
	mywrite (&zero, 1, sizeof(long), outdesc);
      p = p->next;
    }

  for ( i = 0; i < absolutes; i++)
    mywrite (&zero, 1, sizeof(long), outdesc);
}

void
write_global_syms ()
{
  register i;
  int nglobs = 0;
  symbol *sp;
  register char *p, *buf;

  if (!global_syminfo.sp)
    return;
  p = buf = (char *)xmalloc(global_syminfo.symtabsize);
  *(int *)p = global_syminfo.n_globals;
  p += sizeof(long);
  for(i=0;i<TABSIZE;i++)
    for(sp = symtab[i]; sp; sp = sp->link)
      if (sp->defined & N_EXT) {
	nglobs++;
	*(long *)p = sp->value;
	p += sizeof(long);
	*p++ = sp->defined;
	strcpy(p, sp->name);
	p += strlen(sp->name) + 1;
	p = (char *)(round((long)p, sizeof(long)));
      }
  if (nglobs != global_syminfo.n_globals)
    fatal("global_syminfo botch: nglobs=%d, expected=%d\n", nglobs, 
	  global_syminfo.n_globals);
  if ((p - buf) != global_syminfo.symtabsize)
    fatal("global_syminfo botch: p-buf=%d, expected=%d\n", p - buf,
	  global_syminfo.symtabsize);
  mywrite (buf, 1, p - buf, outdesc);
}

#ifdef SHLIB
/* Write the shared library information and initialization tables */
void
write_shlib_syms ()
{
  register int i;
  struct symlist *p;

  p = shlib_info.symlist;
  for ( i = 0; i < shlib_info.n_symlist; i++)
    {
      if (p->sp)
	mywrite (&(p->sp->value), 1, sizeof(long), outdesc);
      else
	mywrite (&(p->sp), 1, sizeof(long), outdesc);
      p = p->next;
    }
  p = shlib_init.symlist;
  for ( i = 0; i < shlib_init.n_symlist; i++)
    {
      if (p->sp)
	mywrite (&(p->sp->value), 1, sizeof(long), outdesc);
      else
	mywrite (&(p->sp), 1, sizeof(long), outdesc);
      p = p->next;
    }
}
#endif SHLIB

/* For relocatable_output only: write out the relocation,
   relocating the addresses-to-be-relocated.  */

void coptxtrel (), copdatrel ();
void coprel ();

void
write_rel ()
{
  register int i;
  register int count = 0;

  /* Assign each global symbol a sequence number, giving the order
     in which `write_syms' will write it.
     This is so we can store the proper symbolnum fields
     in relocation entries we write.  */

  for (i = 0; i < TABSIZE; i++)
    {
      symbol *sp;
      for (sp = symtab[i]; sp; sp = sp->link)
	if (sp->referenced || sp->defined)
	  if (!sp->alias_name)
	    sp->def_count = count++;
    }
  for (i = 0; i < TABSIZE; i++)
    {
      symbol *sp, *alias;
      for (sp = symtab[i]; sp; sp = sp->link)
	if (sp->alias_name)
	  {
	    alias = getsym_soft(sp->alias_name);
	    if (!alias)
	      fatal ("symbol alias: %s not found", sp->alias_name);
	    sp->def_count = alias->def_count;
	  }
    }

  if (count != defined_global_sym_count + undefined_global_sym_count)
    fatal ("internal error");

  if (trace_files)
    fprintf (stderr, "Writing text relocation:\n\n");
#if 0
  each_full_file (coptxtrel);
#else
  for (i = 0; i < number_of_files; i++)
    {
      register struct file_entry *entry = &file_table[i];
      if (entry->just_syms_flag)
	continue;
      if (entry->library_flag)
        {
	  register struct file_entry *subentry = entry->subfiles;
	  for (; subentry; subentry = subentry->chain)
	    coprel(subentry, subentry->text_start_address - text_start,
		  subentry->textrel, subentry->header.a_trsize, SEG_TEXT,
		  SECT_TEXT, 0);
	}
      else
	coprel(entry, entry->text_start_address - text_start, entry->textrel,
	       entry->header.a_trsize, SEG_TEXT, SECT_TEXT, 0);
    }
#endif

  if (trace_files)
    fprintf (stderr, "\nWriting data relocation:\n\n");
#if 0
  each_full_file (copdatrel);
#else
  for (i = 0; i < number_of_files; i++)
    {
      register struct file_entry *entry = &file_table[i];
      if (entry->just_syms_flag)
	continue;
      if (entry->library_flag)
        {
	  register struct file_entry *subentry = entry->subfiles;
	  for (; subentry; subentry = subentry->chain)
	    coprel(subentry, subentry->data_start_address - data_start,
		   subentry->datarel, subentry->header.a_drsize, SEG_DATA,
		   SECT_DATA, 0);
	}
      else
	coprel(entry, entry->data_start_address - data_start, entry->datarel,
	       entry->header.a_drsize, SEG_DATA, SECT_DATA, 0);
    }
#endif

  if (trace_files)
    fprintf (stderr, "Writing (" SEG_OBJC "," SECT_OBJC_SYMBOLS
	     ") relocation:\n\n");
  for (i = 0; i < number_of_files; i++)
    {
      register struct file_entry *entry = &file_table[i];
      if (entry->just_syms_flag)
	continue;
      if (entry->library_flag)
        {
	  register struct file_entry *subentry = entry->subfiles;
	  for (; subentry; subentry = subentry->chain)
	    coprel(subentry, subentry->sym_start_address - objc_start,
		   subentry->symrel, subentry->sym_reloc_size, SEG_OBJC,
		   SECT_OBJC_SYMBOLS, 0);
	}
      else
	coprel(entry, entry->sym_start_address - objc_start, entry->symrel,
	       entry->sym_reloc_size, SEG_OBJC, SECT_OBJC_SYMBOLS, 0);
    }

  if (trace_files)
    fprintf (stderr, "Writing (" SEG_OBJC "," SECT_OBJC_MODULES
	     ") relocation:\n\n");
  for (i = 0; i < number_of_files; i++)
    {
      register struct file_entry *entry = &file_table[i];
      if (entry->just_syms_flag)
	continue;
      if (entry->library_flag)
        {
	  register struct file_entry *subentry = entry->subfiles;
	  for (; subentry; subentry = subentry->chain)
	    coprel(subentry, subentry->mod_start_address -
		   objc_start - sym_size, subentry->modrel,
		   subentry->mod_reloc_size, SEG_OBJC, SECT_OBJC_MODULES, 0);
	}
      else
	coprel(entry, entry->mod_start_address - objc_start - sym_size,
	       entry->modrel, entry->mod_reloc_size, SEG_OBJC,
	       SECT_OBJC_MODULES, 0);
    }

  if (trace_files)
    fprintf (stderr, "Writing (" SEG_OBJC "," SECT_OBJC_STRINGS
	     ") relocation:\n\n");
  for (i = 0; i < number_of_files; i++)
    {
      register struct file_entry *entry = &file_table[i];
      if (entry->just_syms_flag)
	continue;
      if (entry->library_flag)
        {
	  register struct file_entry *subentry = entry->subfiles;
	  for (; subentry; subentry = subentry->chain)
	    coprel(subentry, 0, subentry->selrel, subentry->sel_reloc_size,
		   SEG_OBJC, SECT_OBJC_STRINGS, 1);
	}
      else
	coprel(entry, 0, entry->selrel, entry->sel_reloc_size, SEG_OBJC,
	       SECT_OBJC_STRINGS, 1);
    }

  if (trace_files)
    fprintf (stderr, "\n");
}

void
coprel (entry, reloc, rel, rsize, segname, sectname, map)
     struct file_entry *entry;
     register int reloc;
     register struct relocation_info *rel;
     int rsize;
     char *segname;
     char *sectname;
     int map;
{
  register struct relocation_info *p, *end;

  p = rel;
  end = (struct relocation_info *) (rsize + (char *) p);
  while (p < end)
    {
      if (map)
	p->r_address = map_selector_offset (entry, p->r_address);
      else
	p->r_address += reloc;
      if (p->r_extern)
	{
	  register int symindex = p->r_symbolnum * sizeof (struct nlist);
	  struct nlist *nlp;
	  symbol *symptr;

	  /* This test must be done BEFORE the use of symindex */
	  if (symindex >= entry->header.a_syms)
	    fatal_with_file (entry, "Relocation symbolnum out of range in: ");
	  nlp = (struct nlist *) (((char *)entry->symbols) + symindex);
	  if (((nlp->n_type & N_EXT)) == 0)
	    fatal_with_file (entry, "External relocation entry for "
			     "non-external symbol in: ");
	  symptr = (symbol *)(nlp->n_un.n_name);

	  /* Resolve indirection */
	  if ((symptr->defined & ~N_EXT) == N_INDR)
	    symptr = (symbol *) symptr->value;

	  /*
	   * If the symbol is now defined and not common and not relocatable
	   * output (commons not defined), change the external relocation to
	   * an local one.
 	   */
	  if (symptr->defined &&
	      ((!(symptr->refs->n_type == (N_UNDF | N_EXT) &&
	         symptr->max_common_size != 0) ) ||
	       force_common_definition) )
	    {
	      p->r_extern = 0;
	      if (magic == MH_MAGIC)
		switch (symptr->defined & N_TYPE)
		  {
		  case N_TEXT:
		    p->r_symbolnum = 1;
		    break;
		  case N_DATA:
		    p->r_symbolnum = 2;
		    break;
		  case N_BSS:
		    p->r_symbolnum = 3;
		    break;
		  case N_ABS:
		    p->r_symbolnum = R_ABS;
		    break;
		  case N_SECT:
		    p->r_symbolnum = symptr->sect;
		    break;
		  default:
		    fatal_with_file (entry, "Error in creating local (%s,%s) "
			"relocation symbolnum for %s in: ", segname, sectname,
			symptr->name);
		  }
	      else
		{
		  if ((symptr->defined & N_TYPE) == N_SECT)
		    switch(symptr->sect)
		      {
		      case 1:
			p->r_symbolnum = N_TEXT;
			break;
		      case 2:
			p->r_symbolnum = N_DATA;
			break;
		      case 3:
			p->r_symbolnum = N_BSS;
			break;
		      case 4:
		      case 5:
		      case 6:
			fatal_with_file (entry, "Can't create a.out relocation "
				"entries from Mach-O file with objective-C "
				"sections: ");
		      default:
			fatal_with_file (entry, "Error in creating local "
				"(%s,%s) relocation symbolnum for %s in: ",
				segname, sectname, symptr->name);
		      }
		  else
		    p->r_symbolnum = (symptr->defined & N_TYPE);
		}
	    }
	  else
	    p->r_symbolnum = (symptr->def_count + nsyms
			      - defined_global_sym_count
			      - undefined_global_sym_count);
	}
      else
	{
	  if (entry->mach && magic != MH_MAGIC)
	    {
	      switch(p->r_symbolnum)
		{
		case 1:
		  p->r_symbolnum = N_TEXT;
		  break;
		case 2:
		  p->r_symbolnum = N_DATA;
		  break;
		case 3:
		  p->r_symbolnum = N_BSS;
		  break;
		case 4:
		case 5:
		case 6:
		  fatal_with_file (entry, "Can't create a.out relocation "
			  "entries from Mach-O file with objective-C "
			  "sections: ");
		case R_ABS:
		  p->r_symbolnum = N_ABS;
		  break;
		default:
		  fatal_with_file (entry, "Error in creating local "
			  "(%s,%s) relocation symbolnum for 0x%x in: ",
			  segname, sectname, p->r_symbolnum);
		}
	    }
	  else if (!entry->mach && magic == MH_MAGIC)
	    {
	      switch(p->r_symbolnum)
		{
		case N_TEXT:
		  p->r_symbolnum = 1;
		  break;
		case N_DATA:
		  p->r_symbolnum = 2;
		  break;
		case N_BSS:
		  p->r_symbolnum = 3;
		  break;
		case N_ABS:
		  p->r_symbolnum = R_ABS;
		  break;
		default:
		  fatal_with_file (entry, "Error in creating local "
			  "(%s,%s) relocation symbolnum for 0x%x in: ",
			  segname, sectname, p->r_symbolnum);
		}
	    }
	}
      p++;
    }
  mywrite (rel, 1, rsize, outdesc);
}

#if 0
void
coptxtrel (entry)
     struct file_entry *entry;
{
  register struct relocation_info *p, *end;
  register int reloc = entry->text_start_address;

  p = entry->textrel;
  end = (struct relocation_info *) (entry->header.a_trsize + (char *) p);
  while (p < end)
    {
      p->r_address += reloc;
      if (p->r_extern)
	{
	  register int symindex = p->r_symbolnum * sizeof (struct nlist);
	  struct nlist *nlp;
	  symbol *symptr;

	  /* This test must be done BEFORE the use of symindex */
	  if (symindex >= entry->header.a_syms)
	    fatal_with_file (entry, "Relocation symbolnum out of range in: ");
	  nlp = (struct nlist *) (((char *)entry->symbols) + symindex);
	  if (((nlp->n_type & N_EXT)) == 0)
	    fatal_with_file (entry, "External relocation entry for "
			     "non-external symbol in: ");
	  symptr = (symbol *)(nlp->n_un.n_name);

	  /* Resolve indirection */
	  if ((symptr->defined & ~N_EXT) == N_INDR)
	    symptr = (symbol *) symptr->value;

	  /*
	   * If the symbol is now defined and not common and not relocatable
	   * output (commons not defined), change the external relocation to
	   * an local one.
 	   */
	  if (symptr->defined &&
	      ((!(symptr->refs->n_type == (N_UNDF | N_EXT) &&
	         symptr->max_common_size != 0) ) ||
	       force_common_definition) )
	    {
	      p->r_extern = 0;
	      if (magic == MH_MAGIC)
		switch (symptr->defined & N_TYPE)
		  {
		  case N_TEXT:
		    p->r_symbolnum = 1;
		    break;
		  case N_DATA:
		    p->r_symbolnum = 2;
		    break;
		  case N_BSS:
		    p->r_symbolnum = 3;
		    break;
		  case N_ABS:
		    p->r_symbolnum = R_ABS;
		    break;
		  case N_SECT:
		    p->r_symbolnum = symptr->sect;
		    break;
		  default:
		    fatal ("Internal error in creating local text relocation "
			   "symbolnum for %s", symptr->name);
		  }
	      else
		{
		  if ((symptr->defined & N_TYPE) == N_SECT)
		    switch(symptr->sect)
		      {
		      case 1:
			p->r_symbolnum = N_TEXT;
			break;
		      case 2:
			p->r_symbolnum = N_DATA;
			break;
		      case 3:
			p->r_symbolnum = N_BSS;
			break;
		      default:
			fatal ("Internal error in creating local text "
			       "relocation symbolnum for %s", symptr->name);
		      }
		  else
		    p->r_symbolnum = (symptr->defined & N_TYPE);
		}
	    }
	  else
	    p->r_symbolnum = (symptr->def_count + nsyms
			      - defined_global_sym_count
			      - undefined_global_sym_count);
	}
      else
	{
	  if (entry->mach && magic != MH_MAGIC)
	    {
	      switch(p->r_symbolnum)
		{
		case 1:
		  p->r_symbolnum = N_TEXT;
		  break;
		case 2:
		  p->r_symbolnum = N_DATA;
		  break;
		case 3:
		  p->r_symbolnum = N_BSS;
		  break;
		case R_ABS:
		  p->r_symbolnum = N_ABS;
		  break;
		default:
		  fatal ("Internal error in converting local text "
			 "relocation symbolnum %d", p->r_symbolnum);
		}
	    }
	  else if (!entry->mach && magic == MH_MAGIC)
	    {
	      switch(p->r_symbolnum)
		{
		case N_TEXT:
		  p->r_symbolnum = 1;
		  break;
		case N_DATA:
		  p->r_symbolnum = 2;
		  break;
		case N_BSS:
		  p->r_symbolnum = 3;
		  break;
		case N_ABS:
		  p->r_symbolnum = R_ABS;
		  break;
		default:
		  fatal ("Internal error in converting local text "
			 "relocation symbolnum %d", p->r_symbolnum);
		}
	    }
	}
      p++;
    }
  mywrite (entry->textrel, 1, entry->header.a_trsize, outdesc);
}

#ifdef SHLIB
/* Note that for shared library output data relocation entries are
   not correct.  This wasn't done because a shared library file is never seen as
   input to the link editor and thus the relocation entries are not needed.
   What would have to be done here to get them correct would be
   to adjust the r_address with the entry->global_data_start_address or
   entry->static_data_start_address depending on what type of data it is 
   refering to.  This could easily be done in perform_relocation where it
   should be instead of here as a another pass over the relocation entries. */
#endif SHLIB

void
copdatrel (entry)
     struct file_entry *entry;
{
  register struct relocation_info *p, *end;
  /* Relocate the address of the relocation.
     Old address is relative to start of the input file's data section.
     New address is relative to start of the output file's data section.  */
  register int reloc = entry->data_start_address - text_size;

  p = entry->datarel;
  end = (struct relocation_info *) (entry->header.a_drsize + (char *) p);
  while (p < end)
    {
      p->r_address += reloc;
      if (p->r_extern)
	{
	  register int symindex = p->r_symbolnum * sizeof (struct nlist);
	  struct nlist *nlp;
	  symbol *symptr;

	  /* This test must be done BEFORE the use of symindex */
	  if (symindex >= entry->header.a_syms)
	    fatal_with_file (entry, "Relocation symbolnum out of range in: ");
	  nlp = (struct nlist *) (((char *)entry->symbols) + symindex);
	  if (((nlp->n_type & N_EXT)) == 0)
	    fatal_with_file (entry, "External relocation entry for "
			     "non-external symbol in: ");
	  symptr = (symbol *)(nlp->n_un.n_name);

	  /* Resolve indirection */
	  if ((symptr->defined & ~N_EXT) == N_INDR)
	    symptr = (symbol *) symptr->value;

	  /*
	   * If the symbol is now defined and not common and not relocatable
	   * output (commons not defined), change the external relocation to
	   * an local one.
 	   */
	  if (symptr->defined &&
	      ((!(symptr->refs->n_type == (N_UNDF | N_EXT) &&
	         symptr->max_common_size != 0) ) ||
	       force_common_definition) )
	    {
	      p->r_extern = 0;
	      if (magic == MH_MAGIC)
		switch (symptr->defined & N_TYPE)
		  {
		  case N_TEXT:
		    p->r_symbolnum = 1;
		    break;
		  case N_DATA:
		    p->r_symbolnum = 2;
		    break;
		  case N_BSS:
		    p->r_symbolnum = 3;
		    break;
		  case N_ABS:
		    p->r_symbolnum = R_ABS;
		    break;
		  case N_SECT:
		    p->r_symbolnum = symptr->sect;
		    break;
		  default:
		    fatal ("Internal error in creating local data relocation "
			   "symbolnum for %s", symptr->name);
		  }
	      else
		{
		  if ((symptr->defined & N_TYPE) == N_SECT)
		    switch(symptr->sect)
		      {
		      case 1:
			p->r_symbolnum = N_TEXT;
			break;
		      case 2:
			p->r_symbolnum = N_DATA;
			break;
		      case 3:
			p->r_symbolnum = N_BSS;
			break;
		      default:
			fatal ("Internal error in creating local data "
			       "relocation symbolnum for %s", symptr->name);
		      }
		  else
		    p->r_symbolnum = (symptr->defined & N_TYPE);
		}
	    }
	  else
	    p->r_symbolnum = (symptr->def_count + nsyms
			      - defined_global_sym_count
			      - undefined_global_sym_count);
	}
      else
	{
	  if (entry->mach && magic != MH_MAGIC)
	    {
	      switch(p->r_symbolnum)
		{
		case 1:
		  p->r_symbolnum = N_TEXT;
		  break;
		case 2:
		  p->r_symbolnum = N_DATA;
		  break;
		case 3:
		  p->r_symbolnum = N_BSS;
		  break;
		case R_ABS:
		  p->r_symbolnum = N_ABS;
		  break;
		default:
		  fatal ("Internal error in converting local data "
			 "relocation symbolnum %d", p->r_symbolnum);
		}
	    }
	  else if (!entry->mach && magic == MH_MAGIC)
	    {
	      switch(p->r_symbolnum)
		{
		case N_TEXT:
		  p->r_symbolnum = 1;
		  break;
		case N_DATA:
		  p->r_symbolnum = 2;
		  break;
		case N_BSS:
		  p->r_symbolnum = 3;
		  break;
		case N_ABS:
		  p->r_symbolnum = R_ABS;
		  break;
		default:
		  fatal ("Internal error in converting local data "
			 "relocation symbolnum %d", p->r_symbolnum);
		}
	    }
	}
      p++;
    }
  mywrite (entry->datarel, 1, entry->header.a_drsize, outdesc);
}
#endif 0

void write_file_syms ();
void write_string_table ();

/* Offsets and current lengths of symbol and string tables in output file. */

int symbol_table_offset;
int symbol_table_len;

/* Address in output file where string table starts.  */
int string_table_offset;

/* Offset within string table
   where the strings in `strtab_vector' should be written.  */
int string_table_len;

/* Total size of string table strings allocated so far,
   including strings in `strtab_vector'.  */
int strtab_size;

/* Vector whose elements are strings to be added to the string table.  */
char **strtab_vector;

/* Vector whose elements are the lengths of those strings.  */
int *strtab_lens;

/* Index in `strtab_vector' at which the next string will be stored.  */
int strtab_index;

/* Add the string NAME to the output file string table.
   Record it in `strtab_vector' to be output later.
   Return the index within the string table that this string will have.  */

int
assign_string_table_index (name)
     char *name;
{
  register int index = strtab_size;
  register int len = strlen (name) + 1;

  strtab_size += len;
  strtab_vector[strtab_index] = name;
  strtab_lens[strtab_index++] = len;

  return index;
}

FILE *outstream = (FILE *) 0;

/* Write the contents of `strtab_vector' into the string table.
   This is done once for each file's local&debugger symbols
   and once for the global symbols.  */

void
write_string_table ()
{
  register int i;

  lseek (outdesc, string_table_offset + string_table_len, 0);

  if (!outstream)
    outstream = fdopen (outdesc, "w");

  for (i = 0; i < strtab_index; i++)
    {
      fwrite (strtab_vector[i], 1, strtab_lens[i], outstream);
      string_table_len += strtab_lens[i];
    }

  fflush (outstream);

  /* Report I/O error such as disk full.  */
  if (ferror (outstream))
    perror_fatal ("Can't write to output file: %s", output_filename);
}

/* Write the symbol table and string table of the output file.  */

void
write_syms ()
{
  /* Number of symbols written so far.  */
  int syms_written = 0;
  register int i;
  register symbol *sp;

  /* Buffer big enough for all the global symbols.  */
  struct nlist *buf
    = (struct nlist *) xmalloc ((defined_global_sym_count + undefined_global_sym_count)
			       * sizeof (struct nlist));
  /* Pointer for storing into BUF.  */
  register struct nlist *bufp = buf;

  if (strip_symbols == STRIP_ALL)
    return;

  /* Size of string table includes the bytes that store the size.  */
  strtab_size = sizeof strtab_size;

#ifdef MACH_O
  if (magic == MH_MAGIC)
    symbol_table_offset = stp->symoff;
  else
#endif MACH_O
    symbol_table_offset = N_SYMOFF (outheader);
  symbol_table_len = 0;
#ifdef MACH_O
  if (magic == MH_MAGIC)
    string_table_offset = stp->stroff;
  else
#endif MACH_O
    string_table_offset = N_STROFF (outheader);
  string_table_len = strtab_size;


  /* Write the local symbols defined by the various files.  */

  each_file (write_file_syms, &syms_written);
  file_close ();

  /* Now write out the global symbols.  */

  /* Allocate two vectors that record the data to generate the string
     table from the global symbols written so far.  */

  strtab_vector = (char **) xmalloc ((num_hash_tab_syms + global_indirect_count)
				    * sizeof (char *));
  strtab_lens = (int *) xmalloc ((num_hash_tab_syms + global_indirect_count)
				* sizeof (int));
  strtab_index = 0;

  /* Scan the symbol hash table, bucket by bucket.  */

  for (i = 0; i < TABSIZE; i++)
    for (sp = symtab[i]; sp; sp = sp->link)
      {
	struct nlist nl;

	nl.n_desc = 0;
	/* Compute a `struct nlist' for the symbol.  */

	if (sp->defined || sp->referenced)
	  {
	    /* common condition needs to be before undefined condition */
	    /* because unallocated commons are set undefined in */
	    /* digest_symbols */
	    if (sp->defined > 1) /* defined with known type */
	      {
		/* If the target of an indirect symbol has been
		   defined and we are outputting an executable,
		   resolve the indirection; it's no longer needed */
		if ( ((sp->defined & N_TYPE) == N_INDR) &&
		     (((symbol *) sp->value)->defined > 1) )
		  {
		    symbol *newsp = (symbol *) sp->value;
		    nl.n_type = newsp->defined;
		    nl.n_sect = newsp->sect;
		    nl.n_value = newsp->value;
		  }
		else
		  {
		    nl.n_type = sp->defined;
		    nl.n_sect = sp->sect;
		    if (sp->defined != (N_INDR | N_EXT))
		      nl.n_value = sp->value;
		    else
		      nl.n_value = assign_string_table_index (
				   ((symbol *)(sp->value))->name );
		  }
	      }
	    else if (sp->max_common_size) /* defined as common but not allocated. */
	      {
		/* happens only with -r and not -d */
		/* write out a common definition */
		nl.n_type = N_UNDF | N_EXT;
		nl.n_sect = NO_SECT;
		nl.n_value = sp->max_common_size;
	      }
	    else if (!sp->defined)	      /* undefined -- legit only if -r */
	      {
		nl.n_type = N_UNDF | N_EXT;
		nl.n_sect = NO_SECT;
		nl.n_value = 0;
	      }
	    else
	      fatal ("internal error: %s defined in mysterious way", sp->name);

	    /* Allocate string table space for the symbol name.  */

	    nl.n_un.n_strx = assign_string_table_index (sp->name);

	    /* Output to the buffer and count it.  */

	    *bufp++ = nl;
	    syms_written++;
	  }
      }

  /* Output the buffer full of `struct nlist's.  */

  lseek (outdesc, symbol_table_offset + symbol_table_len, 0);
  mywrite (buf, sizeof (struct nlist), bufp - buf, outdesc);
  symbol_table_len += sizeof (struct nlist) * (bufp - buf);

  if (syms_written != nsyms)
    fatal ("internal error: wrong number of symbols written into output file");

  if (symbol_table_offset + symbol_table_len != string_table_offset)
    fatal ("internal error: inconsistent symbol table length");

  /* Now the total string table size is known, so write it.
     We are already positioned at the right place in the file.  */

  mywrite (&strtab_size, sizeof (int), 1, outdesc);  /* we're at right place */

  /* Write the strings for the global symbols.  */

  write_string_table ();

  free (strtab_vector);
  free (strtab_lens);
  free (buf);
}

/* Write the local and debugger symbols of file ENTRY.
   Increment *SYMS_WRITTEN_ADDR for each symbol that is written.  */

/* Note that we do not combine identical names of local symbols.
   dbx or gdb would be confused if we did that.  */

void
write_file_syms (entry, syms_written_addr)
     struct file_entry *entry;
     int *syms_written_addr;
{
  register struct nlist *p = entry->symbols;
  register struct nlist *end = p + entry->header.a_syms / sizeof (struct nlist);

  /* Buffer to accumulate all the syms before writing them.
     It has one extra slot for the local symbol we generate here.  */
  struct nlist *buf
    = (struct nlist *) xmalloc (entry->header.a_syms + sizeof (struct nlist));
  register struct nlist *bufp = buf;

  /* Upper bound on number of syms to be written here.  */
  int max_syms = (entry->header.a_syms / sizeof (struct nlist)) + 1;

  /* Make tables that record, for each symbol, its name and its name's length.
     The elements are filled in by `assign_string_table_index'.  */

  strtab_vector = (char **) xmalloc (max_syms * sizeof (char *));
  strtab_lens = (int *) xmalloc (max_syms * sizeof (int));
  strtab_index = 0;

  /* Generate a local symbol for the start of this file's text.  */

  if (discard_locals != DISCARD_ALL)
    {
      struct nlist nl;

      if (mach_syms)
	{
	  nl.n_type = N_SECT;
	  nl.n_sect = 1;
	}
      else
	{
	  nl.n_type = N_TEXT;
	  nl.n_sect = 0;
	}
      nl.n_un.n_strx = assign_string_table_index (entry->local_sym_name);
      nl.n_value = entry->text_start_address;
      nl.n_desc = 0;
      *bufp++ = nl;
      (*syms_written_addr)++;
      entry->local_syms_offset = *syms_written_addr * sizeof (struct nlist);
    }

  /* Read the file's string table.  */

  entry->strings = (char *) xmalloc (entry->string_size);
  read_entry_strings (file_open (entry), entry);

  for (; p < end; p++)
    {
      register int type = p->n_type;
      register int write = 0;

      /* WRITE gets 1 for a non-global symbol that should be written.  */

      if (!(type & (N_STAB | N_EXT)))
        /* ordinary local symbol */
	write = (discard_locals != DISCARD_ALL)
		&& !(discard_locals == DISCARD_L &&
		     (p->n_un.n_strx + entry->strings)[0] == 'L');
      else if (!(type & N_EXT))
	/* debugger symbol */
        write = (strip_symbols == STRIP_NONE);

      if (write)
	{
	  /* If this symbol has a name,
	     allocate space for it in the output string table.  */

	  if (p->n_un.n_strx)
	    p->n_un.n_strx = assign_string_table_index (p->n_un.n_strx +
							entry->strings);

	  /* Output this symbol to the buffer and count it.  */

	  *bufp++ = *p;
	  (*syms_written_addr)++;
	}
    }

  /* All the symbols are now in BUF; write them.  */

  lseek (outdesc, symbol_table_offset + symbol_table_len, 0); 
  mywrite (buf, sizeof (struct nlist), bufp - buf, outdesc);
  symbol_table_len += sizeof (struct nlist) * (bufp - buf);

  /* Write the string-table data for the symbols just written,
     using the data in vectors `strtab_vector' and `strtab_lens'.  */

  write_string_table ();

  free (entry->strings);
  free (strtab_vector);
  free (strtab_lens);
  free (buf);
}

#ifndef NO_SYMSEGS
/* Copy any GDB symbol segments from the input files to the output file.
   The contents of the symbol segment is copied without change
   except that we store some information into the beginning of it.  */

void write_file_symseg ();

write_symsegs ()
{
  if (strip_symbols == STRIP_ALL)
    return;

  each_file (write_file_symseg, 0);
  if (lazy_gdb_symtab && used_lazy)
    write_gdb_common_file_info();
#ifdef SYM_ALIAS
  write_alias_sym_info();
#endif SYM_ALIAS
#ifdef SHLIB
  if (shlib_create)
    each_file (write_file_symseg, 1);
#endif SHLIB
}

char *current_dir;

void
write_file_symseg (entry, flag)
     struct file_entry *entry;
     int flag;
{
  char buffer[4096];
  struct symbol_root_header header;
  struct symbol_root root;
  struct mach_root mach_root;
  int indesc, len, total;
  struct ldmap {
    int nmaps;
    int mapoffsets[6];
  } ldmap;
  struct map maps[6];

  if (entry->symseg_offset == 0)
    return;

  /* This entry has a symbol segment.  Read the root header of the segment.  */
  indesc = file_open (entry);
  lseek (indesc, entry->starting_offset + entry->symseg_offset, 0);
  len = read (indesc, &header, sizeof(struct symbol_root_header));
  if (len != sizeof(struct symbol_root_header))
    fatal_with_file (entry, "Premature end of file in symbol segment of: ");
  if (header.format != SYMBOL_ROOT_FORMAT &&
      header.format != MACH_ROOT_FORMAT)
    fatal_with_file (entry, "Unknown symbol segment format of: ");

  /*
   * lazy evaluation symbol table information:
   * ld saves the above information and the filename and offset of the
   * real symbol_root so that the debugger can evaluate it on demand.
   */
  if (lazy_gdb_symtab && flag != 1)
    {
      struct indirect_root *indirect;
      struct mach_indirect_root *mach_indirect;
      struct stat stat_buf;
      long name_malloc_ed = 0;
      char *name;

      used_lazy = 1;/* Need common roots, we are using lazy symtabs */
      if (entry->superfile) {
	name = (char *)xmalloc(strlen(entry->superfile->filename) + 
		      strlen(entry->filename) + 4);
	sprintf(name, "%s(%s)", entry->superfile->filename, entry->filename);
	name_malloc_ed = 1;
      } else {
	/* Not a library member. */
	name = entry->filename;
      }
      if (*name != '/') {
	char *newname;
	/* name starts in current directory.. Prepend current_directory. */
	if (!current_dir) {
	  current_dir = (char *)getwd(buffer);
	  current_dir = concat(current_dir, "/", "");
	}
	newname = concat(current_dir, name, "");
	if(name_malloc_ed)
	  free(name);
	name = newname;
	name_malloc_ed = 1;
      }
#ifdef SHLIB
      if (shlib_create)
	{
	  struct shlib_root *shlib;
	  struct mach_shlib_root *mach_shlib;
	  static int symreloffset = 0;

	  if (header.format == SYMBOL_ROOT_FORMAT)
	    {
	      len = sizeof(struct shlib_root) + strlen(name);
	      len = round(len, sizeof(long));
	      shlib = (struct shlib_root *)xmalloc(len);
	      bzero(shlib, len);
	      shlib->format = SHLIB_ROOT_FORMAT;
	      shlib->length = len;
	      shlib->ldsymoff = entry->local_syms_offset;
	      shlib->textrel = entry->text_start_address;
	      shlib->globaldatarel = entry->global_data_start_address -
				     entry->first_global_data_address;
	      shlib->staticdatarel = entry->static_data_start_address -
				     entry->header.a_text;
	      shlib->globaldatabeg = entry->first_global_data_address;
	      shlib->staticdatabeg = entry->header.a_text;
	      shlib->globaldatasize = entry->header.a_text +
				      entry->header.a_data -
				      entry->first_global_data_address;
	      shlib->staticdatasize = entry->first_global_data_address -
				      entry->header.a_text;
	      shlib->symreloffset =  symreloffset;
	      symreloffset += header.length;
	      strcpy(shlib->filename, name);
	      mywrite (shlib, len, 1, outdesc);
	      free(shlib);
	      if(name_malloc_ed)
	        free(name);
	      return;
	    }
	  else
	    {
	      len = sizeof(struct mach_shlib_root) + strlen(name);
	      len = round(len, sizeof(long));
	      mach_shlib = (struct mach_shlib_root *)xmalloc(len);
	      bzero(mach_shlib, len);
	      mach_shlib->format = MACH_SHLIB_ROOT_FORMAT;
	      mach_shlib->length = len + sizeof(struct ldmap) + sizeof(maps);
	      mach_shlib->ldsymoff = entry->local_syms_offset;
	      mach_shlib->loadmap = (struct loadmap *)len;
	      mach_shlib->symreloffset =  symreloffset;
	      symreloffset += header.length +
			      sizeof(struct ldmap) + sizeof(maps);
	      strcpy(mach_shlib->filename, name);
	      mywrite(mach_shlib, len, 1, outdesc);
	      free(mach_shlib);
	      if(name_malloc_ed)
		free(name);

	      ldmap.nmaps = 6;
	      ldmap.mapoffsets[0] = len + sizeof(struct ldmap) +
				    0 * sizeof(struct map);
	      ldmap.mapoffsets[1] = len + sizeof(struct ldmap) +
				    1 * sizeof(struct map);
	      ldmap.mapoffsets[2] = len + sizeof(struct ldmap) +
				    2 * sizeof(struct map);
	      ldmap.mapoffsets[3] = len + sizeof(struct ldmap) +
				    3 * sizeof(struct map);
	      ldmap.mapoffsets[4] = len + sizeof(struct ldmap) +
				    4 * sizeof(struct map);
	      ldmap.mapoffsets[5] = len + sizeof(struct ldmap) +
				    5 * sizeof(struct map);
	      mywrite((char *)&ldmap, sizeof(struct ldmap), 1, outdesc);
	      maps[0].ldaddr = entry->text_start_address;
	      maps[0].reladdr = 0;
	      maps[0].size = entry->header.a_text;
	      maps[1].ldaddr = entry->global_data_start_address;
	      maps[1].reladdr = entry->first_global_data_address;
	      maps[1].size = entry->first_global_data_address -
			     entry->header.a_text;
	      maps[2].ldaddr = entry->static_data_start_address;
	      maps[2].reladdr = entry->header.a_text;
	      maps[2].size = entry->header.a_text + entry->header.a_data -
			     entry->first_global_data_address;
	      maps[3].ldaddr = entry->sym_start_address;
	      maps[3].reladdr = entry->sym_addr;
	      maps[3].size = entry->sym_size;
	      maps[4].ldaddr = entry->mod_start_address;
	      maps[4].reladdr = entry->mod_addr;
	      maps[4].size = entry->mod_size;
	      maps[5].ldaddr = /* entry->sel_start_address */ 0;
	      maps[5].reladdr = entry->sel_addr;
	      maps[5].size = entry->sel_size;
	      mywrite((char *)maps, sizeof(maps), 1, outdesc);
	      return;
	    }
	}
#endif SHLIB
      if (header.format == SYMBOL_ROOT_FORMAT)
	{
	  len = sizeof(struct indirect_root) + strlen(name);
	  len = round(len, sizeof(long));
	  indirect = (struct indirect_root *)xmalloc(len);
	  bzero(indirect, len);
	  indirect->format = INDIRECT_ROOT_FORMAT;
	  indirect->length = len;
	  indirect->ldsymoff = entry->local_syms_offset;
	  indirect->textrel = entry->text_start_address;
	  indirect->datarel = entry->data_start_address - entry->header.a_text;
	  indirect->bssrel = entry->bss_start_address - entry->bss_addr;
	  indirect->textsize = entry->header.a_text;
	  indirect->datasize = entry->header.a_data;
	  indirect->bsssize = entry->header.a_bss;
	  if (fstat(indesc, &stat_buf) == -1)
	    perror_fatal_with_file (entry, "Can't stat file: ");
	  if (bflag)
	    indirect->mtime = 0;
	  else
	    indirect->mtime = stat_buf.st_mtime;
	  indirect->fileoffset =  entry->symseg_offset + entry->starting_offset;
	  strcpy(indirect->filename, name);
	  mywrite (indirect, len, 1, outdesc);
	  free(indirect);
	  if(name_malloc_ed)
	    free(name);
	  return;
	}
      else
	{
	  len = sizeof(struct mach_indirect_root) + strlen(name);
	  len = round(len, sizeof(long));
	  mach_indirect = (struct mach_indirect_root *)xmalloc(len);
	  bzero(mach_indirect, len);
	  mach_indirect->format = MACH_INDIRECT_ROOT_FORMAT;
	  mach_indirect->length = len + sizeof(struct ldmap) + sizeof(maps);
	  mach_indirect->ldsymoff = entry->local_syms_offset;
	  mach_indirect->loadmap = (struct loadmap *)len;
	  if (fstat(indesc, &stat_buf) == -1)
	    perror_fatal_with_file (entry, "Can't stat file: ");
	  if (bflag)
	    mach_indirect->mtime = 0;
	  else
	    mach_indirect->mtime = stat_buf.st_mtime;
	  mach_indirect->fileoffset = entry->starting_offset +
				      entry->symseg_offset;
	  strcpy(mach_indirect->filename, name);
	  mywrite (mach_indirect, len, 1, outdesc);
	  free(mach_indirect);

	  ldmap.nmaps = 6;
	  ldmap.mapoffsets[0] = len + sizeof(struct ldmap) +
				0 * sizeof(struct map);
	  ldmap.mapoffsets[1] = len + sizeof(struct ldmap) +
				1 * sizeof(struct map);
	  ldmap.mapoffsets[2] = len + sizeof(struct ldmap) +
				2 * sizeof(struct map);
	  ldmap.mapoffsets[3] = len + sizeof(struct ldmap) +
				3 * sizeof(struct map);
	  ldmap.mapoffsets[4] = len + sizeof(struct ldmap) +
				4 * sizeof(struct map);
	  ldmap.mapoffsets[5] = len + sizeof(struct ldmap) +
				5 * sizeof(struct map);
	  mywrite((char *)&ldmap, sizeof(struct ldmap), 1, outdesc);
	  maps[0].ldaddr = entry->text_start_address;
	  maps[0].reladdr = 0;
	  maps[0].size = entry->header.a_text;
	  maps[1].ldaddr = entry->data_start_address;
	  maps[1].reladdr = entry->header.a_text;
	  maps[1].size = entry->header.a_data;
	  maps[2].ldaddr = entry->bss_start_address;
	  maps[2].reladdr = entry->bss_addr;
	  maps[2].size = entry->header.a_bss;
	  maps[3].ldaddr = entry->sym_start_address;
	  maps[3].reladdr = entry->sym_addr;
	  maps[3].size = entry->sym_size;
	  maps[4].ldaddr = entry->mod_start_address;
	  maps[4].reladdr = entry->mod_addr;
	  maps[4].size = entry->mod_size;
	  maps[5].ldaddr = /* entry->sel_start_address */ 0;
	  maps[5].reladdr = entry->sel_addr;
	  maps[5].size = entry->sel_size;
	  mywrite(maps, sizeof(maps), 1, outdesc);
	  if(name_malloc_ed)
	    free(name);
	  return;
	}
    }

  lseek (indesc, entry->starting_offset + entry->symseg_offset, 0);
  if (header.format == SYMBOL_ROOT_FORMAT)
    {
      len = read (indesc, &root, sizeof(struct symbol_root));
      if (len != sizeof(struct symbol_root))
	fatal_with_file (entry, "Premature end of file in symbol segment of: ");
      root.ldsymoff = entry->local_syms_offset;
      root.textrel = entry->text_start_address;
      root.datarel = entry->data_start_address - entry->header.a_text;
      root.bssrel = entry->bss_start_address - entry->bss_addr;
      root.databeg = entry->data_start_address - root.datarel;
      root.bssbeg = entry->bss_start_address - root.bssrel;
      mywrite (&root, sizeof(struct symbol_root), 1, outdesc);
      total = entry->total_size - entry->symseg_offset -
	      sizeof(struct symbol_root);
    }
  else
    {
      len = read (indesc, &mach_root, sizeof(struct mach_root));
      if (len != sizeof(struct mach_root))
	fatal_with_file (entry, "Premature end of file in symbol segment of: ");
      len = mach_root.length;
      mach_root.loadmap = (struct loadmap *)len;
      mach_root.length += sizeof(struct ldmap) + sizeof(maps);
      mywrite (&mach_root, sizeof(struct mach_root), 1, outdesc);
      total = entry->total_size - entry->symseg_offset -
	      sizeof(struct mach_root);
    }

  /* Copy the rest of the symbol segment unchanged.  */
  if (entry->superfile)
    {
      int lenght;
      while (total > 0)
	{

	  lenght = read (indesc, buffer, min (sizeof buffer, total));

	  if (lenght != min (sizeof buffer, total))
	    fatal_with_file (entry,
			     "Premature end of file in symbol segment of: ");
	  total -= lenght;
	  mywrite (buffer, lenght, 1, outdesc);
	}
    }
  else
    {
      /* A separate file: copy until end of file.  */
      int lenght;

      while (lenght = read (indesc, buffer, sizeof buffer))
	{
	  mywrite (buffer, lenght, 1, outdesc);
	  if (lenght < sizeof buffer)
	    break;
	}
    }

  if (header.format == MACH_ROOT_FORMAT)
    {
      ldmap.nmaps = 6;
      ldmap.mapoffsets[0] = len + sizeof(struct ldmap) +
			    0 * sizeof(struct map);
      ldmap.mapoffsets[1] = len + sizeof(struct ldmap) +
			    1 * sizeof(struct map);
      ldmap.mapoffsets[2] = len + sizeof(struct ldmap) +
			    2 * sizeof(struct map);
      ldmap.mapoffsets[3] = len + sizeof(struct ldmap) +
			    3 * sizeof(struct map);
      ldmap.mapoffsets[4] = len + sizeof(struct ldmap) +
			    4 * sizeof(struct map);
      ldmap.mapoffsets[5] = len + sizeof(struct ldmap) +
			    5 * sizeof(struct map);
      mywrite((char *)&ldmap, sizeof(struct ldmap), 1, outdesc);
      maps[0].ldaddr = entry->text_start_address;
      maps[0].reladdr = 0;
      maps[0].size = entry->header.a_text;
      maps[1].ldaddr = entry->data_start_address;
      maps[1].reladdr = entry->header.a_text;
      maps[1].size = entry->header.a_data;
      maps[2].ldaddr = entry->bss_start_address;
      maps[2].reladdr = entry->bss_addr;
      maps[2].size = entry->header.a_bss;
      maps[3].ldaddr = entry->sym_start_address;
      maps[3].reladdr = entry->sym_addr;
      maps[3].size = entry->sym_size;
      maps[4].ldaddr = entry->mod_start_address;
      maps[4].reladdr = entry->mod_addr;
      maps[4].size = entry->mod_size;
      maps[5].ldaddr = /* entry->sel_start_address */ 0;
      maps[5].reladdr = entry->sel_addr;
      maps[5].size = entry->sel_size;
      mywrite((char *)maps, sizeof(maps), 1, outdesc);
    }

  file_close ();
}

struct common_sym {
  symbol *sp;
  struct common_sym *next;
};

struct common_file {
  struct file_entry *entry;
  char *name;
  struct common_sym *csym;
  int nsyms;
  int symstrsize;
  struct common_file *next;
} *common_file_table;

struct common_file *
find_common_file(entry)
struct file_entry *entry;
{
  struct common_file *f;
  int size;
  char *buffer[MAXPATHLEN];

  /* Look file entry in current table */
  f = common_file_table;
  while(f) {
    if (f->entry == entry)
      return(f);
    f = f->next;
  }
  /* Add it to the table. */
  f = (struct common_file *)xmalloc(sizeof(struct common_file));
  f->csym = 0;
  f->nsyms = 0;
  f->symstrsize = 0;
  f->entry = entry;
  if (entry->superfile) {
    f->name = (char *)xmalloc(strlen(entry->superfile->filename) + 
		    strlen(entry->filename) + 4);
    sprintf(f->name, "%s(%s)", entry->superfile->filename, entry->filename);
  } else {
    /* Not a library member. */
    f->name = entry->filename;
  }
  if (*f->name != '/') {
    /* name starts in current directory.. Prepend current_directory. */
    if (!current_dir) {
      current_dir = (char *)getwd(buffer);
      current_dir = concat(current_dir, "/", "");
    }
    f->name = concat(current_dir, f->name, "");
  }
  f->next = common_file_table;
  common_file_table = f;
  return(f);
}

write_gdb_common_file_info() 
{
  register i;
  struct common_file *f;

  /* First build the table */
  for (i = 0; i < TABSIZE; i++)
    {
      register symbol *sp;
      for (sp = symtab[i]; sp; sp = sp->link)
	{
	  if (sp->common_file_entry) {
	    /* Add it to the table */
	    struct common_sym *csym = (struct common_sym *)xmalloc(
		sizeof (struct common_sym));
	    struct common_file *f = find_common_file (sp->common_file_entry);
	    csym->sp = sp;
	    csym->next = f->csym;
	    f->csym = csym;
	    f->nsyms++;
	    f->symstrsize += strlen(sp->name) + 1;
	  }
	}
    }
    /* Now dump it */
    f = common_file_table;
    while(f) {
      int length;
      struct common_root *info;
      register char *p;
      struct common_sym *csym;
      
      length = sizeof (struct common_root) + strlen(f->name) + 1 +
	       f->symstrsize;
      length = round(length, sizeof(long));
      info = (struct common_root *)xmalloc(length);
      bzero (info, length);
      info->format = COMMON_ROOT_FORMAT;
      info->nsyms = f->nsyms;
      info->length = length;
      p = info->data;
      strcpy(p, f->name);
      while(*p)
	 p++;
      p++;
      csym = f->csym;
      while(csym) {
	strcpy(p, csym->sp->name);
	while(*p)
	  p++;
	p++;
	csym = csym->next;
      }
      mywrite (info, length, 1, outdesc);
      free (info);
      f = f->next;
    }
}
#ifdef SYM_ALIAS
write_alias_sym_info ()
{
  register int i, naliases, length;
  register symbol *sp;
  struct alias_root *a;
  char *p;

  if (relocatable_output)
    return;

  length = 0;
  naliases = 0;
  for (sp = removed_aliased_symbols; sp; sp = sp->link)
    {
      naliases++;
      length += strlen(sp->name) + 1;
      length += strlen(sp->alias_name) + 1;
    }
  if (naliases == 0)
    return;
  length += sizeof(struct alias_root);
  length = round(length, sizeof(long));
  a = (struct alias_root *)xmalloc(length);
  bzero(a, length);
  a->format = ALIAS_ROOT_FORMAT;
  a->length = length;
  a->naliases = naliases;

  p = a->data;
  for (sp = removed_aliased_symbols; sp; sp = sp->link)
    {
      strcpy(p, sp->name);
      p += strlen(sp->name) + 1;
      strcpy(p, sp->alias_name);
      p += strlen(sp->alias_name) + 1;
    }
  mywrite (a, length, 1, outdesc);
  free(a);
}
#endif SYM_ALIAS

#endif
/* Compute the hash code for symbol name KEY.  */

int
hash_string (key)
     char *key;
{
  register char *cp;
  register int k;

  cp = key;
  k = 0;
  while (*cp)
    k = (((k << 1) + (k >> 14)) ^ (*cp++)) & 0x3fff;

  return k;
}

/* Get the symbol table entry for the global symbol named KEY.
   Create one if there is none.  */

symbol *
getsym (key)
     char *key;
{
  register int hashval;
  register symbol *bp;

  /* Determine the proper bucket.  */

  hashval = hash_string (key) % TABSIZE;

  /* Search the bucket.  */

  for (bp = symtab[hashval]; bp; bp = bp->link)
    if (! strcmp (key, bp->name))
      return bp;

  /* Nothing was found; create a new symbol table entry.  */

  bp = (symbol *) xmalloc (sizeof (symbol));
  bp->refs = 0;
  bp->name = (char *) xmalloc (strlen (key) + 1);
  bp->alias_name = (char *) 0;
  strcpy (bp->name, key);
  bp->defined = 0;
  bp->sect = 0;
  bp->referenced = 0;
  bp->trace = 0;
  bp->value = 0;
  bp->max_common_size = 0;
  bp->common_file_entry = 0;

  /* Add the entry to the bucket.  */

  bp->link = symtab[hashval];
  symtab[hashval] = bp;

  ++num_hash_tab_syms;

  return bp;
}

/* Like `getsym' but return 0 if the symbol is not already known.  */

symbol *
getsym_soft (key)
     char *key;
{
  register int hashval;
  register symbol *bp;

  /* Determine which bucket.  */

  hashval = hash_string (key) % TABSIZE;

  /* Search the bucket.  */

  for (bp = symtab[hashval]; bp; bp = bp->link)
    if (! strcmp (key, bp->name))
      return bp;

  return 0;
}

/* Get the offset in the output file for the named SELECTOR.  If the selector
   is not in the selector output file yet added it.  */

int
getsel (selector)
     char *selector;
{
  int hashval, len;
  struct sel_bucket *bp;
  struct sel_block **p, *sel_block;

/*
printf("in getsel(%s)\n", selector);
*/
  if (seltab == NULL)
    {
      seltab = (struct sel_bucket **) xmalloc (sizeof(struct sel_bucket *) *
					       SELTABSIZE);
      bzero(seltab, sizeof(struct sel_bucket *) * SELTABSIZE);
    }
  /* Determine the proper bucket.  */
  hashval = hash_string (selector) % SELTABSIZE;

  /* Search the buckets.  */
  for (bp = seltab[hashval]; bp; bp = bp->next)
    if (! strcmp (selector, bp->sel_name))
      return (bp->offset);

  /* Nothing was found; create a selector bucket and add the selector string
     to the blocks that contain them for the output file.  */
  bp = (struct sel_bucket *) xmalloc (sizeof(struct sel_bucket));

  len = strlen(selector) + 1;
  for (p = &sel_blocks; *p ; p = &(sel_block->next))
    {
      sel_block = *p;
      if (sel_block->full)
	continue;
      if (len > sel_block->size - sel_block->used)
	{
	  sel_block->full = 1;
	  continue;
	}
      strcpy(sel_block->sels + sel_block->used, selector);
      bp->sel_name = sel_block->sels + sel_block->used;
      sel_block->used += len;
      bp->offset = sel_size;
      bp->next = seltab[hashval];
      seltab[hashval] = bp;
      sel_size += len;
      return (bp->offset);
    }
  *p = (struct sel_block *) xmalloc (sizeof(struct sel_block));
  sel_block = *p;
  sel_block->size = (len > page_size ? len : page_size);
  sel_block->used = len;
  sel_block->full = (len == sel_block->size ? 1 : 0);
  sel_block->next = NULL;
  sel_block->sels = (char *) xmalloc (sel_block->size);
  strcpy(sel_block->sels, selector);
  bp->sel_name = sel_block->sels;
  bp->offset = sel_size;
  bp->next = seltab[hashval];
  seltab[hashval] = bp;
  sel_size += len;
  return (bp->offset);
}

/* Get the offset in the output file for the offset INPUT_OFFSET in the input
   file for ENTRY.  */

map_selector_offset (entry, input_offset)
    struct file_entry *entry;
    long input_offset;
{
    int l = 0;
    int u = entry->nsels - 1;
    int m;
    int r;

    if (input_offset < 0 || input_offset > entry->sel_size) 
	fatal_with_file(entry, "Reference to %s section outside it's bounds",
			SECT_OBJC_STRINGS);

    while (l <= u) {
	m = (l + u) / 2;
	if ((r = (input_offset - entry->sel_map[m].input_offset)) == 0) {
	    return (entry->sel_map[m].output_offset);
	} else if (r < 0) {
	    u = m - 1;
	} else {
	    l = m + 1;
	}
    }
/*
printf("In map_selector_offset(%d) (in=%d,out=%d)\n", input_offset,
entry->sel_map[m-1].input_offset,
entry->sel_map[m-1].output_offset);
*/
    if (m == 0 || input_offset > entry->sel_map[m].input_offset)
	return (entry->sel_map[m].output_offset +
		input_offset - entry->sel_map[m].input_offset);
    else
	return (entry->sel_map[m-1].output_offset +
		input_offset - entry->sel_map[m-1].input_offset);
}


/* Report a fatal error.
   The arguments are the same as printf  */

fatal(va_alist) /* (char *fmt, ...) */
    va_dcl
{
  char *fmt;
  va_list ap;

  va_start(ap);
  fprintf(stderr, "%s: ", LOADER);
  fmt = va_arg(ap, char *);
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  va_end(ap);
  exit(1);
}

/* Report a fatal error but don't exit.  Instead set increment fatal_errors.
   The arguments are the same as printf  */

fatal_error(va_alist) /* (char *fmt, ...) */
    va_dcl
{
  char *fmt;
  va_list ap;

  va_start(ap);
  fprintf(stderr, "%s: ", LOADER);
  fmt = va_arg(ap, char *);
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  va_end(ap);
  fatal_errors++;
}

/* Report a fatal error.  The error message is followed by the filename of
   entry.  The rest of the arguments are the same as printf  */

fatal_with_file(va_alist) /* (struct file_entry *entry, char *fmt, ...) */
    va_dcl
{
  struct file_entry *entry;
  char *fmt;
  va_list ap;

  va_start(ap);
  fprintf(stderr, "%s: ", LOADER);
  entry = va_arg(ap, struct file_entry *);
  fmt = va_arg(ap, char *);
  vfprintf(stderr, fmt, ap);
  print_file_name(entry, stderr);
  fprintf(stderr, "\n");
  va_end(ap);
  exit(1);
}

/* Report a fatal error but don't exit.  Instead set fatal_errors.  The error
   message is followed by the filename of entry.  The rest of the arguments
   are the same as printf  */

fatal_error_with_file(va_alist) /* (struct file_entry *entry, char *fmt, ...) */
    va_dcl
{
  struct file_entry *entry;
  char *fmt;
  va_list ap;

  va_start(ap);
  fprintf(stderr, "%s: ", LOADER);
  entry = va_arg(ap, struct file_entry *);
  fmt = va_arg(ap, char *);
  vfprintf(stderr, fmt, ap);
  print_file_name(entry, stderr);
  fprintf(stderr, "\n");
  va_end(ap);
  fatal_errors++;
}

/* Report a fatal error using the message for the last failed system call.
   The arguments are the same as printf  */

perror_fatal(va_alist) /* (char *fmt, ...) */
    va_dcl
{
  char *fmt;
  va_list ap;
  extern int errno, sys_nerr;
  extern char *sys_errlist[];

  va_start(ap);
  fprintf(stderr, "%s: ", LOADER);
  fmt = va_arg(ap, char *);
  vfprintf(stderr, fmt, ap);
  if(errno < sys_nerr)
    fprintf(stderr, " (%s)\n", sys_errlist[errno]);
  else
    fprintf(stderr, " (errno = %d)\n", errno);
  va_end(ap);
  exit(1);
}

/* Report a fatal error using the message for the last failed system call.
   The error message is followed by the filename of entry.  The rest of the
   arguments are the same as printf  */

perror_fatal_with_file(va_alist) /* (struct file_entry *entry, char *fmt, ...)*/
    va_dcl
{
  struct file_entry *entry;
  char *fmt;
  va_list ap;
  extern int errno, sys_nerr;
  extern char *sys_errlist[];

  va_start(ap);
  fprintf(stderr, "%s: ", LOADER);
  entry = va_arg(ap, struct file_entry *);
  fmt = va_arg(ap, char *);
  vfprintf(stderr, fmt, ap);
  print_file_name(entry, stderr);
  if(errno < sys_nerr)
    fprintf(stderr, " (%s)\n", sys_errlist[errno]);
  else
    fprintf(stderr, " (errno = %d)\n", errno);
  va_end(ap);
  exit(1);
}

/* Report a nonfatal error.
   The arguments are the same as printf  */

error(va_alist) /* (char *fmt, ...) */
    va_dcl
{
  char *fmt;
  va_list ap;

  va_start(ap);
  fprintf(stderr, "%s: ", LOADER);
  fmt = va_arg(ap, char *);
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  va_end(ap);
}

/* Report a nonfatal error.  The error message is followed by the filename of
   entry.  The rest of the arguments are the same as printf  */

error_with_file(va_alist) /* (struct file_entry *entry, char *fmt, ...) */
    va_dcl
{
  struct file_entry *entry;
  char *fmt;
  va_list ap;

  va_start(ap);
  fprintf(stderr, "%s: ", LOADER);
  entry = va_arg(ap, struct file_entry *);
  fmt = va_arg(ap, char *);
  vfprintf(stderr, fmt, ap);
  print_file_name(entry, stderr);
  fprintf(stderr, "\n");
  va_end(ap);
}

/* Output COUNT*ELTSIZE bytes of data at BUF
   to the descriptor DESC.  */

mywrite (buf, count, eltsize, desc)
     char *buf;
     int count;
     int eltsize;
     int desc;
{
  register int val;
  register int bytes = count * eltsize;

  while (bytes > 0)
    {
      val = write (desc, buf, bytes);
      if (val <= 0)
	perror_fatal ("Can't write to output file: %s", output_filename);
      buf += val;
      bytes -= val;
    }
}

/* Output PADDING zero-bytes to descriptor OUTDESC.
   PADDING may be negative; in that case, do nothing.  */

void
padfile (padding, outdesc)
     int padding;
     int outdesc;
{
  register char *buf;
  if (padding <= 0)
    return;

  buf = (char *) xmalloc (padding);
  bzero (buf, padding);
  mywrite (buf, padding, 1, outdesc);
  free (buf);
}

/* Return a newly-allocated string
   whose contents concatenate the strings S1, S2, S3.  */

char *
concat (s1, s2, s3)
     char *s1, *s2, *s3;
{
  register int len1 = strlen (s1), len2 = strlen (s2), len3 = strlen (s3);
  register char *result = (char *) xmalloc (len1 + len2 + len3 + 1);

  strcpy (result, s1);
  strcpy (result + len1, s2);
  strcpy (result + len1 + len2, s3);
  result[len1 + len2 + len3] = 0;

  return result;
}

/* Parse the string ARG using scanf format FORMAT, and return the result.
   If it does not parse, report fatal error
   generating the error message using format string ERROR and ARG as arg.  */

int
parse (arg, format, error)
     char *arg, *format;
{
  int x;
  if (1 != sscanf (arg, format, &x))
    fatal (error, arg);
  return x;
}

/* Like malloc but get fatal error if memory is exhausted.  */

int
xmalloc (size)
     int size;
{
  register int result;
  result = malloc (size);

  if (!result)
    fatal ("virtual memory exhausted");
  return result;
}

/* Like realloc but get fatal error if memory is exhausted.  */

int
xrealloc (ptr, size)
     char *ptr;
     int size;
{
  register int result = realloc (ptr, size);
  if (!result)
    fatal ("virtual memory exhausted");
}

/*
 * Round v to a multiple of r.
 */
long
round(
    long v,
    unsigned long r)
{
	r--;
	v += r;
	v &= ~(long)r;
	return(v);
}

