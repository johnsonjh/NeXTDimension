/*
 * This file describes the link editor defined symbols.  The semantics of a link
 * editor is that it is defined by the link editor only if it is referenced and
 * it is an error for the user to define them (see the man page ld(1)).  The
 * standard UNIX link editor symbols are: __end, __etext and __edata as
 * described on the end(3) man page.  Note that the compiler prepends an
 * underbar to all external symbol names coded in a high level language.  Thus
 * in 'C' end is coded without an underbar and its symbol in the symbol table
 * has an underbar.  There are two cpp macros for each link editor defined name
 * in this file.  The macro with a leading underbar is the symbol name and the
 * one without is the name as coded in 'C'.
 */

/*
 * The link editor defined symbols __etext, __edata and __end are defined as
 * follows in a mach-O object file: __etext is that first address after the
 * last non-zero fill section in the __TEXT segment.  __edata is the first
 * address after the last non-zero fill section in the __DATA segment.  And
 * __end is the first address after the last section in the __DATA segment.
 */
#define _ETEXT "__etext"
#define ETEXT "_etext"
#define _EDATA "__edata"
#define EDATA "_edata"
#define _END "__end"
#define END "_end"

/*
 * Along with the above link editor defined names are names for the beginning
 * and ending of each segment and each section in each segment in a mach-O
 * object file.  The names for the symbols for a segment's beginning and end
 * will have the form: __SEGNAME__begin and  __SEGNAME__end where __SEGNAME
 * is the name of the segment.  The names for the symbols for a section's
 * beginning and end will have the form: __SEGNAME__sectname__begin and 
 * __SEGNAME__sectname__end where __sectname is the name of the section and
 * __SEGNAME is the segment it is in.
 * 
 * Both these symbols and and the standard UNIX link editor symbols are not
 * part of shared libraries but rather the executable that uses then and thus
 * are NOT defined in the shared library.
 */

/*
 * The above symbols' types are those of the section they are refering to.
 * This is true even for symbols who's values are end's of a section and
 * that value is next address after that section and not really in that
 * section.  This results in these symbols having types refering to sections
 * who's values are not in that section.
 */

/*
 * The link editor defined symbol _MH_EXECUTE_SYM is the address of the mach
 * header in a mach-O executable file type.  It does not appear in any other
 * file type.
 */
#define _MH_EXECUTE_SYM	"__mh_execute_header"
#define MH_EXECUTE_SYM	"_mh_execute_header"
extern const struct mach_header *_mh_execute_header;

/*
 * The link editor defined symbol _OBJC_BIND_SYM is the objective C 4.0 module
 * descriptor table.  The table is pairs of symbol values whos names starts
 * with _OBJC_LINK and _OBJC_INFO and have the same suffix.  The table ends
 * with two zero values.  The table is long word (32 bit) aligned in the data
 * segment.
 */
#define _OBJC_BIND_SYM	"__objcModuleDescriptorTable"
#define OBJC_BIND_SYM	"_objcModuleDescriptorTable"
#define _OBJC_LINK	"__BIND_"
#define OBJC_LINK	"_BIND_"
#define _OBJC_INFO	"__modDesc__BIND_"
#define OBJC_INFO	"_modDesc__BIND_"

/*
 * The link editor defined symbol _MOD_DESC_IND_TAB is the module descriptor
 * indirect table.  This is used by objective-C runtime instead of the above
 * symbol as of the 1.0 release.  This table contains pointers to module
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
#define _MOD_DESC_IND_TAB	"__NX_modDescIndirectTable"
#define MOD_DESC_IND_TAB	"_NX_modDescIndirectTable"
#define _MOD_DESC_TAB		"__NX_modDescTable_"
#define MOD_DESC_TAB		"_NX_modDescTable_"
#define _MOD_DESC		"__NX_modDesc_"
#define MOD_DESC		"_NX_modDesc_"

/*
 * The link editor defined symbol _SHLIB_INFO is the shared library information
 * table.  It is a pointer to an array of shared library structures.  This link
 * editor defined symbol is only used in the runtime startup for a.out format
 * files using shared libraries.  This is no longer used as since mach-O object
 * files get their shared libraries map my the kernel on exec(2).
 */
#define _SHLIB_INFO	"__shared_library_information"
#define SHLIB_INFO	"_shared_library_information"
extern struct shlib_info *_shared_library_information[];

/*
 * The prefix to the library definition symbol name.  The base name of the
 * target object file is the rest of the name.  The link editor looks for
 * these names if the link editor symbol _SHLIB_INFO is referenced and builds
 * a table of the addresses of shared library symbol names for the link editor
 * defined symbol (zero terminated).   The link editor also looks for these
 * symbols when building mach-O executables and mach-O shared libraries and
 * reads the data for these symbols from the relocatable files to propagate the
 * target names into the load commands.  These symbols MUST be shlib_info
 * structs.  At some point in the future these symbol will go away and the
 * information will be propagated by mach-O load commands.
 */
#define SHLIB_SYMBOL_NAME	".shared_library_information_"

#define MAX_TARGET_SHLIB_NAME_LEN	64
struct shlib_info {
    long text_addr;
    long text_size;
    long data_addr;
    long data_size;
    long minor_version;
    char name[MAX_TARGET_SHLIB_NAME_LEN];
};
#define SHLIB_STRUCT \
"	.long %d\n \
	.long %d\n \
	.long %d\n \
	.long %d\n \
	.long %d\n \
	.ascii \"%s\\0\"\n \
	.skip %d\n"

/*
 * The link editor defined symbol _SHLIB_INIT is the shared library
 * initialization table.  It is a pointer to an array of pointers that
 * point at arrays of shared library initialization structures.
 */
#define _SHLIB_INIT	"__shared_library_initialization"
#define SHLIB_INIT	"_shared_library_initialization"
extern struct shlib_init *_shared_library_initialization[];

/*
 * The prefix to the symbol names of the arrays of shared library initialization
 * structures.  Again the base name of the object file is the rest of the name.
 * Each of these arrays' last structure must have zeroes in all its fields.
 * The link editor looks for these symbol names if the link editor symbol
 * _SHLIB_INIT is referenced and builds a table of the values of init symbol
 * names for the link editor defined symbol (zero terminated).
 */
#define INIT_SYMBOL_NAME	".shared_library_initialization_"

struct shlib_init {
    long value;		/* the value to be stored at the address */
    long *address;	/* the address to store the value */
};

/*
 * The prefix to the symbol names for branch table slots in target shared
 * libraries. The rest of the symbol name is digits.  This symbol does not
 * appear in host shared libraries and thus does not appear in executables
 * that use them.
 */
#define BRANCH_SLOT_NAME	".branch_table_slot_"

/*
 * Symbol which kicks on loading the global symbol table by dynaload.
 */
#define LOAD_GLOBAL_SYMTAB_INFO "__globalSymtabInfo_"
