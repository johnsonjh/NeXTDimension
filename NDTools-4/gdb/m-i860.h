/* Parameters for execution on the Intel I860 for GDB, the GNU debugger.
   Copyright (C) 1986, 1987 Free Software Foundation, Inc.

GDB is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY.  No author or distributor accepts responsibility to anyone
for the consequences of using it or for whether it serves any
particular purpose or works at all, unless he says so in writing.
Refer to the GDB General Public License for full details.

Everyone is granted permission to copy, modify and redistribute GDB,
but only under the conditions described in the GDB General Public
License.  A copy of this license is supposed to have been given to you
along with GDB so you can know your rights and responsibilities.  It
should be in a file named COPYING.  Among other things, the copyright
notice and this notice must be preserved on all copies.

In other words, go ahead and share GDB, but don't try to stop
anyone else from sharing it farther.  Help stamp out software hoarding!
*/

#ifndef i860
#define i860 1
#endif

#define HAVE_TERMIO
#define USG
#define SVR4
#define vfork fork
#define sigsetmask(x) 

#define FALSE 0
#define TRUE 1

/* Define this if the C compiler puts an underscore at the front
   of external names before giving them to the linker.  */

#define NAMES_HAVE_UNDERSCORE

/* Debugger information will be in COFF format.  */

#define COFF_FORMAT
#define COFF_NO_LONG_FILE_NAMES

/* Offset from address of function to start of its code.
   Zero on most machines.  */

#define FUNCTION_START_OFFSET 0

/* The call instruction puts its return address in register r1 and does
   not change the stack pointer */

#define RETURN_ADDR_IN_REGISTER

/* Advance PC across any function entry prologue instructions
   to reach some "real" code.  */

#define SKIP_PROLOGUE(pc) \
  { pc = skip_prologue (pc); }

/* Immediately after a function call, return the saved pc.
   Can't go through the frames for this because on some machines
   the new frame is not set up until the new function executes
   some instructions.  */

#define SAVED_PC_AFTER_CALL(frame) \
	(read_register(RP_REGNUM))

/* Address of end of stack space.  */

#define STACK_END_ADDR 0x80000000

/* Stack grows downward.  */

#define INNER_THAN <

/* Stack has strict alignment.  */

#define STACK_ALIGN(ADDR) (((ADDR)+15)&-16)

/* Sequence of bytes for breakpoint instruction.  */

#define BREAKPOINT {0x00, 0x00, 0x00, 0x44}

/* Amount PC must be decremented by after a breakpoint.
   This is often the number of bytes in BREAKPOINT
   but not always.  */

#define DECR_PC_AFTER_BREAK 0

/* Nonzero if instruction at PC is a return instruction.  */
/* should be "bri r1"   */

#define ABOUT_TO_RETURN(pc) \
  (read_memory_integer (pc, 4) == 0x400000800)

/* Return 1 if P points to an invalid floating point value.  */

#define INVALID_FLOAT(p, len) 1   /* Just a first guess; not checked */

/* Largest integer type */
#define LONGEST long

/* Name of the builtin type for the LONGEST type above. */
#define BUILTIN_TYPE_LONGEST builtin_type_long

/* Say how long (ordinary) registers are.  */

#define REGISTER_TYPE long

/* Number of machine registers */

#define NUM_REGS 68

/* Initializer for an array of names of registers.
   There should be NUM_REGS strings in this initializer.  */

#define REGISTER_NAMES  \
{								\
  "r0",  "r1",  "sp",  "fp",  "r4",  "r5",  "r6",  "r7",	\
  "r8",  "r9",  "r10", "r11", "r12", "r13", "r14", "r15",	\
  "r16", "r17", "r18", "r19", "r20", "r21", "r22", "r23",	\
  "r24", "r25", "r26", "r27", "r28", "r29", "r30", "r31",	\
								\
  "f0",  "f1",  "f2",  "f3",  "f4",  "f5",  "f6",  "f7",	\
  "f8",  "f9",  "f10", "f11", "f12", "f13", "f14", "f15",	\
  "f16", "f17", "f18", "f19", "f20", "f21", "f22", "f23",	\
  "f24", "f25", "f26", "f27", "f28", "f29", "f30", "f31",	\
								\
  "pc", "psr", "fpsr", "db"					\
};
/* Register numbers of various important registers.
   Note that some of these values are "real" register numbers,
   and correspond to the general registers of the machine,
   and some are "phony" register numbers which are too large
   to be actual register numbers as far as the user is concerned
   but do serve to get the desired values when passed to read_register.  */

#define RP_REGNUM 1		/* Contains return address value */
#define SP_REGNUM 2		/* Contains address of top of stack, */
#define FP_REGNUM 3		/* Contains address of executing stack frame */
				/* which is also the bottom of the frame.  */
#define Y_REGNUM 31		/* Temp register for address calc., etc.  */
#define PC_REGNUM 64		/* Contains program counter */
#define PS_REGNUM 65		/* Contains processor status */
#define FP0_REGNUM 32		/* Floating point register 0 */
#define FPS_REGNUM 66		/* Floating point status register */
#define DB_REGNUM 67		/* Debug register */

/* Total amount of space needed to store our copies of the machine's
   register state, the array `registers'.  */
#define REGISTER_BYTES (NUM_REGS * 4)

/* Index within `registers' of the first byte of the space for
   register N.  */

#define REGISTER_BYTE(N) ((N)*4)

/* Number of bytes of storage in the actual machine representation
   for register N.  */

/* On the i860, all regs are 4 bytes.  */

#define REGISTER_RAW_SIZE(N) (4)

/* Number of bytes of storage in the program's representation
   for register N.  */

/* On the i860, all regs are 4 bytes.  */

#define REGISTER_VIRTUAL_SIZE(N) (4)

/* Largest value REGISTER_RAW_SIZE can have.  */

#define MAX_REGISTER_RAW_SIZE 8

/* Largest value REGISTER_VIRTUAL_SIZE can have.  */

#define MAX_REGISTER_VIRTUAL_SIZE 8

/* Nonzero if register N requires conversion
   from raw format to virtual format.  */

#define REGISTER_CONVERTIBLE(N) (0)

/* Convert data from raw format for register REGNUM
   to virtual format for register REGNUM.  */

#define REGISTER_CONVERT_TO_VIRTUAL(REGNUM,FROM,TO) \
{ bcopy ((FROM), (TO), 4); }

/* Convert data from virtual format for register REGNUM
   to raw format for register REGNUM.  */

#define REGISTER_CONVERT_TO_RAW(REGNUM,FROM,TO)	\
{ bcopy ((FROM), (TO), 4); }

/* Return the GDB type object for the "standard" data type
   of data in register N.  */

#define REGISTER_VIRTUAL_TYPE(N) \
 ((N) < 32 ? builtin_type_int : (N) < 64 ? builtin_type_float : builtin_type_int)

/* Store the address of the place in which to copy the structure the
   subroutine will return.  This is called from call_function. */

#define STORE_STRUCT_RETURN(ADDR, SP)  { write_register (16, (ADDR)); }

/* The following need work for float types */

/* Extract from an array REGBUF containing the (raw) register state
   a function return value of type TYPE, and copy that, in virtual format,
   into VALBUF.  */

#define EXTRACT_RETURN_VALUE(TYPE,REGBUF,VALBUF) \
  bcopy ((char *) (REGBUF) + REGISTER_BYTE(16), (VALBUF), TYPE_LENGTH (TYPE))

/* Write into appropriate registers a function return value
   of type TYPE, given in virtual format.  */
/* On i860, values are returned in register r16.  */
#define STORE_RETURN_VALUE(TYPE,VALBUF) \
  write_register_bytes (REGISTER_BYTE (16), VALBUF, TYPE_LENGTH (TYPE))

/* Extract from an array REGBUF containing the (raw) register state
   the address in which a function should return its structure value,
   as a CORE_ADDR (or an expression that can be used as one).  */

#define EXTRACT_STRUCT_VALUE_ADDRESS(REGBUF) \
  (*(int *) ((REGBUF) + REGISTER_BYTE(16)))


/* Describe the pointer in each stack frame to the previous stack frame
   (its caller).  */
#include <sys/reg.h>


/* FRAME_CHAIN takes a frame's nominal address
   and produces the frame's chain-pointer.

   FRAME_CHAIN_COMBINE takes the chain pointer and the frame's nominal address
   and produces the nominal address of the caller frame.

   However, if FRAME_CHAIN_VALID returns zero,
   it means the given frame is the outermost one and has no caller.
   In that case, FRAME_CHAIN_COMBINE is not used.  */

/* In the case of the i860, the frame-chain's nominal address
   is held in the frame pointer register.

   On the i860 the frame (in %fp) points to %fp for the previous frame.
 */

/* our FRAME_CHAIN requires a pointer to all the frame info (e.g. pc)
 * so it's called FRAME_CHAIN_POINTER
 */

#define FRAME_CHAIN_POINTER(thisframe)  frame_chain(thisframe)

#define FRAME_CHAIN_VALID(chain, thisframe) \
  (chain != 0 && FRAME_SAVED_PC (thisframe) >= first_object_file_end)

#define FRAME_CHAIN_COMBINE(chain, thisframe) (chain)

/* Define other aspects of the stack frame.  */

#define FRAME_SAVED_PC(frame) frame_saved_pc(frame)

#define FRAME_ARGS_ADDRESS(fi) ((fi)->frame)

#define FRAME_LOCALS_ADDRESS(fi) ((fi)->frame)

/* Set VAL to the number of args passed to frame described by FI.
   Can set VAL to -1, meaning no way to tell.  */

/* We can't tell how many args there are */
 
#define FRAME_NUM_ARGS(val,fi) (val = -1)

#define FRAME_STRUCT_ARGS_ADDRESS(fi) ((fi)->frame)

/* Return number of bytes at start of arglist that are not really args.  */

#define FRAME_ARGS_SKIP 8

/* Put here the code to store, into a struct frame_saved_regs,
   the addresses of the saved registers of frame described by FRAME_INFO.
   This includes special registers such as pc and fp saved in special
   ways in the stack frame.  sp is even more special:
   the address we return for it IS the sp for the next frame.  */

/* Grind through the various st.l rx,Y(fp) and fst.z fx,Y(fp) */

#define FRAME_FIND_SAVED_REGS(frame_info, frame_saved_regs)	\
	frame_find_saved_regs(frame_info, &(frame_saved_regs))

/* Things needed for making the inferior call functions.  */

/* Push an empty stack frame, to record the current PC, etc.  */
/* We have this frame with fp pointing to a block where all GDB-visible
   registers are stored in the order GDB knows them, and sp at the next
   alignment point below fp.  Note: fp + NUM_REGS*4 was the old sp
   */

#define PUSH_DUMMY_FRAME \
{ register CORE_ADDR fp = read_register (SP_REGNUM);			\
  extern char registers[];						\
  fp -= REGISTER_BYTES;							\
  write_register(SP_REGNUM, (fp &~ 15));	/* re-align */		\
  write_memory(fp, registers, REGISTER_BYTES);				\
  write_register(FP_REGNUM, fp);}

/* Discard from the stack the innermost frame, 
   restoring all saved registers.  */

#define POP_FRAME  \
{ register FRAME frame = get_current_frame ();			 	      \
  register CORE_ADDR fp;					 	      \
  register int regnum;						 	      \
  struct frame_saved_regs fsr;					 	      \
  struct frame_info *fi;						      \
  char raw_buffer[12];						 	      \
  fi = get_frame_info (frame);					 	      \
  fp = fi->frame;						 	      \
  get_frame_saved_regs (fi, &fsr);				 	      \
  for (regnum = FP0_REGNUM + 31; regnum >= FP0_REGNUM; regnum--)	      \
    if (fsr.regs[regnum])					 	      \
      write_register (regnum, read_memory_integer (fsr.regs[regnum], 4));     \
  for (regnum = 31; regnum >= 1; regnum--)			 	      \
    if (fsr.regs[regnum])					 	      \
      if (regnum != SP_REGNUM)						      \
	write_register (regnum, read_memory_integer (fsr.regs[regnum], 4));   \
      else								      \
	write_register (SP_REGNUM, fsr.regs[SP_REGNUM]);	 	      \
  if (fsr.regs[PS_REGNUM])					 	      \
    write_register (PS_REGNUM, read_memory_integer (fsr.regs[PS_REGNUM], 4)); \
  if (fsr.regs[FPS_REGNUM])					 	      \
    write_register (FPS_REGNUM, read_memory_integer (fsr.regs[FPS_REGNUM],4));\
  if (fsr.regs[PC_REGNUM])					 	      \
    write_register (PC_REGNUM, read_memory_integer (fsr.regs[PC_REGNUM], 4)); \
  flush_cached_frames ();					 	      \
  set_current_frame (create_new_frame (read_register (FP_REGNUM),	      \
					read_pc ())); }

/* This sequence of words is the instructions:
	*../
--
     nop
     calli r31
     nop
     trap r0,r0,r0
--
Note this is 16 bytes.
Saving of the registers is done by PUSH_DUMMY_FRAME.  The address of the
function to call will be placed in register r31 prior to starting.
The arguments have to be put into the parameter registers by GDB after 
the PUSH_DUMMY_FRAME is done.  NOTE: GDB expects to push arguments, but
i860 takes them in registers */

#define CALL_DUMMY { \
	0xa0000000,	0x4c00F802,	0xa0000000,	0x44000000 }


/* This setup is somewhat unusual.  PUSH_DUMMY_FRAME and 
   FRAME_FIND_SAVED_REGS conspire to handle dummy frames differently.
   Therefore, CALL_DUMMY can be minimal. We put the address of the called
   function in r31 and let'er rip */

#define CALL_DUMMY_LENGTH 16

/* Actually start at the calli */

#define CALL_DUMMY_START_OFFSET 4

/* Normally, GDB will patch the dummy code here to put in the function
   address, etc., but we only neefd to put the call adddress in r31 */

#define FIX_CALL_DUMMY(dummyname, pc, fun, nargs, type)	write_register(31, fun)


/* i860 has no reliable single step ptrace call */

#define NO_SINGLE_STEP 1

#define KERNEL_U_ADDR 0xd0000000

#define REGISTER_U_ADDR(addr, ar0, regno)		\
{	if (regno < 64) \
	  addr = ar0 + (regno + R0) * 4; \
	else if (regno == PC_REGNUM) \
	  addr = ar0 + PC * 4; \
	else if (regno == PS_REGNUM) \
	  addr = ar0 + PSR * 4; \
	else if (regno == FPS_REGNUM) \
	  addr = ar0 + PSV_FSR1 * 4; \
	else if (regno == DB_REGNUM) \
	  addr = ar0 + DB * 4; \
}

/* --> For now, we won't handle functions which take more paramaters than
 * there are registers to hold them
 */

/* How many bytes are pushed on the stack for the argument list
 */
#define STACK_ARG_BYTES(RESULT,ARGS,NARGS,STRUCT_RETURN) {(RESULT) = 0;}

/* Pass arguments to a function
 */
#define PASS_FUNCTION_ARGUMENTS(ARGS,NARGS,STRUCT_RETURN)	\
	pass_function_arguments(ARGS,NARGS,STRUCT_RETURN)

/* Put dummy call code at the end of text if possible
 */
#define TEXT_DUMMY

/* Call dummy code location is variable (in stack or in text)
 */
#define CALL_DUMMY_VARIABLE

/* The i860 can have text after code, etc.
 */
#define NONSTANDARD_MEMORY_LAYOUT

/* We support pseudo-frame structure so actual frame pointer value must be 
 * read from the register.
 */
#define FP_FROM_REGISTER

/* Support a quit from info register command because we have so many regs
 */
#define INFO_REGISTER_QUIT

/* Identify text or absolute symbols to put in misc function table.  In the
 * i860 case, only text symbols not starting with _L00
 */
#define MISC_FUNCTION(cs) ((cs)->c_secnum == 1 && (cs)->c_value && \
			strncmp((cs)->c_name, "_L00", 4) != 0)

/* Define our a.out magic number and undefine the 386 one if it's defined
 */
#ifdef I860MAGIC
#define AOUT_MAGIC I860MAGIC
#else
#define AOUT_MAGIC 0515
#endif

#ifdef I386MAGIC
#undef I386MAGIC
#endif

/* The aouthdr may be larger than defined in a.out.h
 */
#define AOUTHDR_VARIABLE

/* We have a bug where lineno information is not in symbol table correctly
 */
#define LINENO_BUG

/* Similarly, the Endndx of a structure definition is bogus in ld860
 */
#define STRUCT_SYMBOL_BUG

/* Also, we see T_ARG types when we shouldn't
 */
#define T_ARG_BUG

/* Macro to sign extend to 32 bits */
#define SIGN_EXT(n,x)	(((int) (x) << (32-n)) >> (32-n))

#define SIGN_EXT16(x)	(((int) (x) << (16)) >> (16))
