/* Machine-dependent code which would otherwise be in inflow.c and core.c,
   for GDB, the GNU debugger.
   Copyright (C) 1986, 1987 Free Software Foundation, Inc.
   This code is for the i860 cpu.

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

#include "defs.h"
#include "param.h"
#include "frame.h"
#include "inferior.h"
#include "obstack.h"
#include "symtab.h"
#include "value.h"

#include "m-i860.h"
#include "i860-opcode.h"

#include <stdio.h>
#ifdef MYUSER_H
#include <sys/param.h>
#include "my.user.h"
#else
#include <sys/types.h>
#include <sys/param.h>
#include <sys/dir.h>
#include <sys/user.h>
#endif

#include <signal.h>
#include <sys/ioctl.h>
#include <fcntl.h>

#include <sys/reg.h>

#include <a.out.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <core.h>

#define NBPG NBPC
#define UPAGES USIZE

extern int errno;
extern int attach_flag;

/* This function simply calls ptrace with the given arguments.  
   It exists so that all calls to ptrace are isolated in this 
   machine-dependent file. */
int
call_ptrace (request, pid, arg3, arg4)
     int request, pid, arg3, arg4;
{
  return ptrace (request, pid, arg3, arg4);
}

void
kill_inferior ()
{
  if (remote_debugging)
    return;
  if (inferior_pid == 0)
    return;
  ptrace (8, inferior_pid, 0, 0);
  wait (0);
  inferior_died ();
}

/* This is used when GDB is exiting.  It gives less chance of error.*/

void
kill_inferior_fast ()
{
  if (remote_debugging)
    return;
  if (inferior_pid == 0)
    return;
  ptrace (8, inferior_pid, 0, 0);
  wait (0);
}

/* Simulate single-step ptrace call for sun4.  Code written by Gary
   Beihl (beihl@mcc.com).  */
/* Modified for i860 by Jim Hanko (hanko@orc.olivetti.com) */

/* 
 * Duplicated from breakpoint.c because (at least for now) this is a
 * machine dependent routine.
 */
static char break_insn[] = BREAKPOINT;


static CORE_ADDR brkpc, brkpc1;
typedef char binsn_quantum[sizeof break_insn];
static binsn_quantum break_mem[2];

/* Non-zero if we just simulated a single-step ptrace call.  This is
   needed because we cannot remove the breakpoints in the inferior
   process until after the `wait' in `wait_for_inferior'.  Used for
   i860. */

int one_stepped;

void
single_step (signal)
     int signal;
{
  branch_type br, isabranch();
  CORE_ADDR pc;

  pc = read_register (PC_REGNUM);

  if (!one_stepped)
    {
      brkpc1 = 0;
      br = isabranch (pc, &brkpc);

      /* Always set breakpoint for brkpc. (brkpc is pc+4 for non-branch,
	 branch target for branches) */

      /* printf ("SS: set break at %x, type %d\n",brkpc, br); */
      read_memory (brkpc, break_mem[0], sizeof break_insn);
      write_memory (brkpc, break_insn, sizeof break_insn);

      if (br == cond && brkpc != pc + 4)
	{ 
	  brkpc1 = pc + 4;

	  /* printf ("SS(cond): set break at %x\n",brkpc1); */
	  read_memory (brkpc1, break_mem[1], sizeof break_insn);
	  write_memory (brkpc1, break_insn, sizeof break_insn);
	}
      if (br == cond_d && brkpc != pc + 8)
	{ 
	  brkpc1 = pc + 8;

	  /* printf ("SS(cond_d): set break at %x\n",brkpc1); */
	  read_memory (brkpc1, break_mem[1], sizeof break_insn);
	  write_memory (brkpc1, break_insn, sizeof break_insn);
	}

      /* Let it go */
      ptrace (7, inferior_pid, 1, signal);
      one_stepped = 1;
      return;
    }
  else
    {
      /* Remove breakpoints */
      write_memory (brkpc, break_mem[0], sizeof break_insn);

      if (brkpc1)
	{
	  write_memory (brkpc1, break_mem[1], sizeof break_insn);
	}
      one_stepped = 0;
    }
}

/* Resume execution of the inferior process.
   If STEP is nonzero, single-step it.
   If SIGNAL is nonzero, give it that signal.  */

void
resume (step, signal)
     int step;
     int signal;
{
  errno = 0;
  if (remote_debugging)
    remote_resume (step, signal);
  else
    {
      /* i860 doesn't have single step on ptrace */
      if (step)
	{
	/* printf("Step at 0x%x\n", read_register (PC_REGNUM)); */
	single_step (signal);
	}
      else
	{
	/* printf("Continue at 0x%x\n", read_register (PC_REGNUM)); */
	ptrace (7, inferior_pid, 1, signal);
	}
      if (errno)
	perror_with_name ("ptrace");
    }
}

void
fetch_inferior_registers ()
{
  register int regno;
  register unsigned int regaddr;
  char buf[MAX_REGISTER_RAW_SIZE];
  register int i;

  struct user u;
  unsigned int offset = (char *) &u.u_ar0 - (char *) &u;
  offset = ptrace (3, inferior_pid, offset, 0) - KERNEL_U_ADDR;

  for (regno = 0; regno < NUM_REGS; regno++)
    {
      regaddr = register_addr (regno, offset);
      for (i = 0; i < REGISTER_RAW_SIZE (regno); i += sizeof (int))
 	{
 	  *(int *) &buf[i] = ptrace (3, inferior_pid, regaddr, 0);
 	  regaddr += sizeof (int);
 	}
      supply_register (regno, buf);
    }
}

/* Store our register values back into the inferior.
   If REGNO is -1, do this for all registers.
   Otherwise, REGNO specifies which register (so we can save time).  */

store_inferior_registers (regno)
     int regno;
{
  register unsigned int regaddr;
  char buf[80];

  struct user u;
  unsigned int offset = (char *) &u.u_ar0 - (char *) &u;
  offset = ptrace (3, inferior_pid, offset, 0) - KERNEL_U_ADDR;

  if (regno >= 0)
    {
      regaddr = register_addr (regno, offset);
      errno = 0;
      ptrace (6, inferior_pid, regaddr, read_register (regno));
      if (errno != 0)
	{
	  sprintf (buf, "writing register number %d", regno);
	  perror_with_name (buf);
	}
    }
  else for (regno = 0; regno < NUM_REGS; regno++)
    {
      regaddr = register_addr (regno, offset);
      errno = 0;
      ptrace (6, inferior_pid, regaddr, read_register (regno));
      if (errno != 0)
	{
	  sprintf (buf, "writing register number %d", regno);
	  perror_with_name (buf);
	}
    }
}

/* Copy LEN bytes from inferior's memory starting at MEMADDR
   to debugger memory starting at MYADDR. 
   On failure (cannot read from inferior, usually because address is out
   of bounds) returns the value of errno. */

int
read_inferior_memory (memaddr, myaddr, len)
     CORE_ADDR memaddr;
     char *myaddr;
     int len;
{
  register int i;
  /* Round starting address down to longword boundary.  */
  register CORE_ADDR addr = memaddr & - sizeof (int);
  /* Round ending address up; get number of longwords that makes.  */
  register int count
    = (((memaddr + len) - addr) + sizeof (int) - 1) / sizeof (int);
  /* Allocate buffer of that many longwords.  */
  register int *buffer = (int *) alloca (count * sizeof (int));
  extern int errno;

  /* Read all the longwords */
  for (i = 0; i < count; i++, addr += sizeof (int))
    {
      errno = 0;
      if (remote_debugging)
	buffer[i] = remote_fetch_word (addr);
      else
	buffer[i] = ptrace (1, inferior_pid, addr, 0);
      if (errno)
	return errno;
    }

  /* Copy appropriate bytes out of the buffer.  */
  bcopy ((char *) buffer + (memaddr & (sizeof (int) - 1)), myaddr, len);
  return 0;
}

/* Copy LEN bytes of data from debugger memory at MYADDR
   to inferior's memory at MEMADDR.
   On failure (cannot write the inferior)
   returns the value of errno.  */

int
write_inferior_memory (memaddr, myaddr, len)
     CORE_ADDR memaddr;
     char *myaddr;
     int len;
{
  register int i;
  /* Round starting address down to longword boundary.  */
  register CORE_ADDR addr = memaddr & - sizeof (int);
  /* Round ending address up; get number of longwords that makes.  */
  register int count
    = (((memaddr + len) - addr) + sizeof (int) - 1) / sizeof (int);
  /* Allocate buffer of that many longwords.  */
  register int *buffer = (int *) alloca (count * sizeof (int));
  extern int errno;
  extern CORE_ADDR text_start;	/* used to distinguish I&D for ptrace */
  extern CORE_ADDR text_end;

  /* Fill start and end extra bytes of buffer with existing memory data.  */

  if (remote_debugging)
    buffer[0] = remote_fetch_word (addr);
  else
    buffer[0] = ptrace (1, inferior_pid, addr, 0);

  if (count > 1)
    {
      if (remote_debugging)
	buffer[count - 1]
	  = remote_fetch_word (addr + (count - 1) * sizeof (int));
      else
	buffer[count - 1]
	  = ptrace (1, inferior_pid,
		    addr + (count - 1) * sizeof (int), 0);
    }

  /* Copy data to be written over corresponding part of buffer */

  bcopy (myaddr, (char *) buffer + (memaddr & (sizeof (int) - 1)), len);

  /* Write the entire buffer.  */

  for (i = 0; i < count; i++, addr += sizeof (int))
    {
      errno = 0;
      if (remote_debugging)
	remote_store_word (addr, buffer[i]);
      else if (addr >= text_start && addr <= text_end + CALL_DUMMY_LENGTH)
	ptrace (4, inferior_pid, addr, buffer[i]);
      else
	ptrace (5, inferior_pid, addr, buffer[i]);
      if (errno)
	return errno;
    }

  return 0;
}


/* Work with core dump and executable files, for GDB. 
   This code would be in core.c if it weren't machine-dependent. */

/* Recognize COFF format systems because a.out.h defines AOUTHDR.  */
#ifdef AOUTHDR
#define COFF_FORMAT
#endif

#ifndef N_TXTADDR
#define N_TXTADDR(hdr) 0
#endif /* no N_TXTADDR */

#ifndef N_DATADDR
#define N_DATADDR(hdr) hdr.a_text
#endif /* no N_DATADDR */

/* Make COFF and non-COFF names for things a little more compatible
   to reduce conditionals later.  */

#ifdef COFF_FORMAT
#define a_magic magic
#endif

#ifndef COFF_FORMAT
#define AOUTHDR struct exec
#endif

extern char *sys_siglist[];


/* Hook for `exec_file_command' command to call.  */

extern void (*exec_file_display_hook) ();
   
/* File names of core file and executable file.  */

extern char *corefile;
extern char *execfile;

/* Descriptors on which core file and executable file are open.
   Note that the execchan is closed when an inferior is created
   and reopened if the inferior dies or is killed.  */

extern int corechan;
extern int execchan;

/* Last modification time of executable file.
   Also used in source.c to compare against mtime of a source file.  */

extern int exec_mtime;

/* Virtual addresses of bounds of the two areas of memory in the core file.  */

extern CORE_ADDR data_start;
extern CORE_ADDR data_end;
extern CORE_ADDR stack_start;
extern CORE_ADDR stack_end;

/* Virtual addresses of bounds of two areas of memory in the exec file.
   Note that the data area in the exec file is used only when there is no core file.  */

extern CORE_ADDR text_start;
extern CORE_ADDR text_end;

extern CORE_ADDR exec_data_start;
extern CORE_ADDR exec_data_end;

/* Address in executable file of start of text area data.  */

extern int text_offset;

/* Address in executable file of start of data area data.  */

extern int exec_data_offset;

/* Address in core file of start of data area data.  */

extern int data_offset;

/* Address in core file of start of stack area data.  */

extern int stack_offset;

#ifdef COFF_FORMAT
/* various coff data structures */

extern FILHDR file_hdr;
extern SCNHDR text_hdr;
extern SCNHDR data_hdr;

#endif /* not COFF_FORMAT */

/* a.out header saved in core file.  */
  
extern AOUTHDR core_aouthdr;

/* a.out header of exec file.  */

extern AOUTHDR exec_aouthdr;

extern void validate_files ();

core_file_command (filename, from_tty)
     char *filename;
     int from_tty;
{
  int val;
  extern char registers[];
  CORE_ADDR fp, sp;

  /* Discard all vestiges of any previous core file
     and mark data and stack spaces as empty.  */

  if (corefile)
    free (corefile);
  corefile = 0;

  if (corechan >= 0)
    close (corechan);
  corechan = -1;

  data_start = 0;
  data_end = 0;
  stack_start = STACK_END_ADDR;
  stack_end = STACK_END_ADDR;

  /* Now, if a new core file was specified, open it and digest it.  */

  if (filename)
    {
      if (have_inferior_p ())
	error ("To look at a core file, you must kill the inferior with \"kill\".");
      corechan = open (filename, O_RDONLY, 0);
      if (corechan < 0)
	perror_with_name (filename);
      /* 4.2-style (and perhaps also sysV-style) core dump file.  */
      {
	struct user u;

	int reg_offset;

	val = myread (corechan, &u, sizeof u);
	if (val < 0)
	  perror_with_name (filename);
	data_start = exec_data_start;
	data_end = data_start + NBPG * u.u_dsize;
	stack_start = stack_end - NBPG * u.u_ssize;
	data_offset = NBPG * UPAGES;
	stack_offset = NBPG * (UPAGES + u.u_dsize);
	reg_offset = (int) u.u_ar0 - KERNEL_U_ADDR;

	/* I don't know where to find this info.
	   So, for now, mark it as not available.  */
	core_aouthdr.a_magic = 0;

	/* Read the register values out of the core file and store
	   them where `read_register' will find them.  */

	{
	  register int regno;

	  for (regno = 0; regno < NUM_REGS; regno++)
	    {
	      char buf[MAX_REGISTER_RAW_SIZE];

	      val = lseek (corechan, register_addr (regno, reg_offset), 0);
	      if (val < 0)
		perror_with_name (filename);

 	      val = myread (corechan, buf, sizeof buf);
	      if (val < 0)
		perror_with_name (filename);
	      supply_register (regno, buf);
	    }
	}
      }
      if (filename[0] == '/')
	corefile = savestring (filename, strlen (filename));
      else
	{
	  corefile = concat (current_directory, "/", filename);
	}

      fp = read_register (FP_REGNUM);
      sp = read_register (SP_REGNUM);

      /* Verify that the fp is in the stack (if we can)
       */
      if (fp < sp || fp > stack_end)
	fp = sp;

      set_current_frame ( create_new_frame ( fp, read_pc ()));
      select_frame (get_current_frame (), 0);
      validate_files ();
    }
  else if (from_tty)
    printf ("No core file now.\n");
}

exec_file_command (filename, from_tty)
     char *filename;
     int from_tty;
{
  int val;

  /* Eliminate all traces of old exec file.
     Mark text segment as empty.  */

  if (execfile)
    free (execfile);
  execfile = 0;
  data_start = 0;
  data_end -= exec_data_start;
  text_start = 0;
  text_end = 0;
  exec_data_start = 0;
  exec_data_end = 0;
  if (execchan >= 0)
    close (execchan);
  execchan = -1;

  /* Now open and digest the file the user requested, if any.  */

  if (filename)
    {
      char *getenv();
      char *path = getenv("PATH");

      if (path == NULL)
	path = "";
      execchan = openp (path, 1, filename, O_RDONLY, 0, &execfile);
      if (execchan < 0)
	perror_with_name (filename);

#ifdef COFF_FORMAT
      {
	int aout_hdrsize;
	int num_sections;

	if (read_file_hdr (execchan, &file_hdr) < 0)
	  error ("\"%s\": not in executable format.", execfile);

	aout_hdrsize = file_hdr.f_opthdr;
	num_sections = file_hdr.f_nscns;

	if (read_aout_hdr (execchan, &exec_aouthdr, aout_hdrsize) < 0)
	  error ("\"%s\": can't read optional aouthdr", execfile);

	if (read_section_hdr (execchan, _TEXT, &text_hdr, num_sections) < 0)
	  error ("\"%s\": can't read text section header", execfile);

	if (read_section_hdr (execchan, _DATA, &data_hdr, num_sections) < 0)
	  error ("\"%s\": can't read data section header", execfile);

	text_start = exec_aouthdr.text_start;
	text_end = text_start + exec_aouthdr.tsize;
	text_offset = text_hdr.s_scnptr;
	exec_data_start = exec_aouthdr.data_start;
	exec_data_end = exec_data_start + exec_aouthdr.dsize;
	exec_data_offset = data_hdr.s_scnptr;
	data_start = exec_data_start;
	data_end += exec_data_start;
	exec_mtime = file_hdr.f_timdat;
      }
#else /* not COFF_FORMAT */
      {
	struct stat st_exec;

	val = myread (execchan, &exec_aouthdr, sizeof (AOUTHDR));

	if (val < 0)
	  perror_with_name (filename);

        text_start = N_TXTADDR (exec_aouthdr);
        exec_data_start = N_DATADDR (exec_aouthdr);

	text_offset = N_TXTOFF (exec_aouthdr);
	exec_data_offset = N_TXTOFF (exec_aouthdr) + exec_aouthdr.a_text;

	text_end = text_start + exec_aouthdr.a_text;
        exec_data_end = exec_data_start + exec_aouthdr.a_data;
	data_start = exec_data_start;
	data_end += exec_data_start;

	fstat (execchan, &st_exec);
	exec_mtime = st_exec.st_mtime;
      }
#endif /* not COFF_FORMAT */

      validate_files ();
    }
  else if (from_tty)
    printf ("No exec file now.\n");

  /* Tell display code (if any) about the changed file name.  */
  if (exec_file_display_hook)
    (*exec_file_display_hook) (filename);
}

/* Written for i860 by Jim Hanko (hanko@orc.olivetti.com) */
/* This code was based on SPARC code written by Gary Beihl (beihl@mcc.com),
    by Michael Tiemann (tiemann@corto.inria.fr).  */

struct command_line *get_breakpoint_commands ();

CORE_ADDR 
skip_prologue (pc)
     CORE_ADDR pc;
{
  long instr;
  int regno;

  instr = read_memory_integer (pc, 4);

  /* Recognize "addu|adds -X,sp,sp" insn. */
  if ((instr & 0xEFFF0000) == 0x84420000)
    {
      pc += 4;
      instr = read_memory_integer (pc, 4);
    }
  else
    return(pc);					/* No frame! */

  /* Recognize store of return addr and frame pointer into frame */
  while (1)
    {
      if ((instr & 0xFFE0F801) == 0x1C400801 ||  /* st.l r1,X(sp) */
          (instr & 0xFFE0F801) == 0x1C401801)    /* st.l fp,X(sp) */
        {
          pc += 4;
          instr = read_memory_integer (pc, 4);
        }
      else
 	break;
    }

  /* Recognize "addu|adds X,sp,fp" insn. */
  if ((instr & 0xEFFF0000) == 0x84430000)
    {
      pc += 4;
      instr = read_memory_integer (pc, 4);
    }

  /* Now recognize stores into the frame from the registers. */

  while (1)
    {
      if ((instr & 0xFFA00003) == 0x1C200001 ||	/* st.l rn,X(fp|sp) */
          (instr & 0xFFA00001) == 0x4C200000)	/* fst.y fn,X(fp|sp) */
        {
	  regno = (instr >> 11) & 0x1f;
	  if (regno == 0)			/* source reg == 0? quit */
	    break;
          pc += 4;
          instr = read_memory_integer (pc, 4);
        }
      else
        break;
    }

  return(pc);
}


/* Set *nextpc to branch target if we find a branch.  If it is not a branch, 
   set it to the next instruction (addr + 4) */

branch_type
isabranch (addr,  nextpc)
     CORE_ADDR addr, *nextpc;
{
  long instr;
  branch_type val = not_branch;
  long offset; /* Must be signed for sign-extend */

  *nextpc = addr + 4;
  instr = read_memory_integer (addr, 4);
  /* printf("instr = %x\n",instr); */

  if ((instr & 0xE0000000) == 0x60000000 &&		/* CTRL format */
      (instr & 0xF8000000) != 0x60000000)		/* not pfld.y  */
    {
      if ((instr & 0xF8000000) == 0x68000000)		/* br or call */
	val = uncond_d;
      else if ((instr & 0xF4000000) == 0x74000000)	/* bc.t or bnc.t */
	val = cond_d;
      else if ((instr & 0xF4000000) == 0x70000000)	/* bc or bnc */
	val = cond;

      offset = (instr & 0x03ffffff);
      if (offset & 0x02000000)	/* sign extend? */
	offset |= 0xFC000000;
      *nextpc = addr + 4 + (offset << 2);
    }
  else if ((instr & 0xFC00003F) == 0x4C000002 ||	/* calli */
           (instr & 0xFC000000) == 0x40000000)		/* bri */
    {
      val = uncond_d;
      offset = ((instr & 0x0000F800) >> 11);
      *nextpc = (read_register(offset) & 0xFFFFFFFC);
    }
  else if ((instr & 0xF0000000) == 0x50000000)		/* bte or btne */
    {
      val = cond;

      offset = SIGN_EXT16(((instr & 0x001F0000) >> 5)  | (instr & 0x000007FF));
      *nextpc = addr + 4 + (offset << 2);
    }
  else if ((instr & 0xFC000000) == 0xB4000000)         /* bla */
    {
      val = cond_d;

      offset = SIGN_EXT16(((instr & 0x001F0000) >> 5)  | (instr & 0x000007FF));
      *nextpc = addr + 4 + (offset << 2);
    }

  /*printf("isabranch ret: %d\n",val); */
  return val;
}

/* set in call_function() [valops.c] to the address of the "call dummy" code
   so dummy frames can be easily recognized; also used in wait_for_inferior() 
   [infrun.c]. When not used, it points into the ABI's 'reserved area' */

CORE_ADDR call_dummy_set = 0;	/* true if dummy call being done */
CORE_ADDR call_dummy_start;	/* address of call dummy code */

frame_find_saved_regs(frame_info, frame_saved_regs)
    struct frame_info *frame_info;
    struct frame_saved_regs *frame_saved_regs;
{
  register CORE_ADDR pc;
  long instr, spdelta = 0, offset;
  int i, size, reg;
  int r1_off = -1, fp_off = -1;
  int framesize;

  bzero (frame_saved_regs, sizeof(*frame_saved_regs));

  if (call_dummy_set && frame_info->pc >= call_dummy_start && 
	frame_info->pc <= call_dummy_start + CALL_DUMMY_LENGTH)
    {
      /* DUMMY frame - all registers stored in order at fp; old sp is
	 at fp + NUM_REGS*4 */

      for (i = 1; i < NUM_REGS; i++) /* skip reg 0 */
	if (i != SP_REGNUM && i != FP0_REGNUM && i != FP0_REGNUM + 1)
	  frame_saved_regs->regs[i] = frame_info->frame + i*4;

      frame_saved_regs->regs[SP_REGNUM] = frame_info->frame + NUM_REGS*4;

      call_dummy_set = 0;
      return;
    }

  pc = get_pc_function_start (frame_info->pc); 

  instr = read_memory_integer (pc, 4);
  /* Recognize "addu|adds -X,sp,sp" insn. */
  if ((instr & 0xEFFF0000) == 0x84420000)
    {
      framesize = -SIGN_EXT16(instr & 0x0000FFFF);
      pc += 4;
      instr = read_memory_integer (pc, 4);
    }
  else
    goto punt;					/* No frame! */

  /* Recognize store of return addr and frame pointer into frame */
  while (1)
    {
      if ((instr & 0xFFE0F801) == 0x1C400801)  /* st.l r1,X(sp) */
        {
	  r1_off = SIGN_EXT16(((instr&0x001F0000) >> 5) | (instr&0x000007FE));
          pc += 4;
          instr = read_memory_integer (pc, 4);
        }
      else if ((instr & 0xFFE0F801) == 0x1C401801)    /* st.l fp,X(sp) */
        {
	  fp_off = SIGN_EXT16(((instr&0x001F0000) >> 5) | (instr&0x000007FE));
          pc += 4;
          instr = read_memory_integer (pc, 4);
        }
      else
 	break;
    }

  /* Recognize "addu|adds X,sp,fp" insn. */
  if ((instr & 0xEFFF0000) == 0x84430000)
    {
      spdelta = SIGN_EXT16(instr & 0x0000FFFF);
      pc += 4;
      instr = read_memory_integer (pc, 4);
    }

  /* Now recognize stores into the frame from the registers. */

  while (1)
    {
      if ((instr & 0xFFC00003) == 0x1C400001)	/* st.l rn,X(fp|sp) */
        {
	  offset = SIGN_EXT16(((instr&0x001F0000) >> 5) | (instr&0x000007FE));
	  reg = (instr >> 11) & 0x1F;
	  if (reg == 0)
	    break;
	  if ((instr & 0x00200000) == 0)	/* was this using sp? */
	    if (spdelta)			/* and we know sp-fp delta */
	      offset -= spdelta;		/* if so, adjust the offset */
	    else
	      break;				/* if not, give up */


	  /* Handle the case where the return address is stored after the fp 
	     is adjusted */

	  if (reg == 1)
	    frame_saved_regs->regs[PC_REGNUM] = frame_info->frame + offset;
	  else
	    frame_saved_regs->regs[reg] = frame_info->frame + offset;

          pc += 4;
          instr = read_memory_integer (pc, 4);
        }
      else if ((instr & 0xFFC00001) == 0x2C400000) /* fst.y fn,X(fp|sp) */
        {
	  /*
	   * The number of words in a floating store based on 3 LSB of instr
	   */
	  static int fst_sizes[] = {2, 0, 1, 0, 4, 0, 1, 0};

	  size = fst_sizes[instr & 7];
	  reg = ((instr >> 16) & 0x1F) + FP0_REGNUM;
	  if (reg == 0)
	    break;

	  if (size > 1)					/* align the offset */
	    offset = SIGN_EXT16(instr & 0x0000FFF8);	/* drop 3 bits */
	  else
	    offset = SIGN_EXT16(instr & 0x0000FFFC);	/* drop 2 bits */

	  if ((instr & 0x00200000) == 0)	/* was this using sp? */
	    if (spdelta)			/* and we know sp-fp delta */
	      offset -= spdelta;		/* if so, adjust the offset */
	    else
	      break;				/* if not, give up */

	  for (i = 0; i < size; i++)
	    {
	      frame_saved_regs->regs[reg] = frame_info->frame + offset;

	      offset += 4;
	      reg++;
	    }

          pc += 4;
          instr = read_memory_integer (pc, 4);
        }
      else
        break;
    }

punt: ;
  if (framesize != 0 && spdelta != 0)
    frame_saved_regs->regs[SP_REGNUM] = frame_info->frame+(framesize-spdelta);
  else
    frame_saved_regs->regs[SP_REGNUM] = frame_info->frame + 8;

  if (spdelta && fp_off != -1)
    frame_saved_regs->regs[FP_REGNUM] = frame_info->frame - spdelta + fp_off;
  else
    frame_saved_regs->regs[FP_REGNUM] = frame_info->frame;

  if (spdelta && r1_off != -1)
    frame_saved_regs->regs[PC_REGNUM] = frame_info->frame - spdelta + r1_off;
  else
    frame_saved_regs->regs[PC_REGNUM] = frame_info->frame + 4;
}


frame_chain(thisframe)
    FRAME thisframe;
{
  unsigned long fp, sp;
  unsigned long func_start;
  long instr;
  int offset;
  unsigned long thisfp = thisframe->frame;


  fp = read_memory_integer (thisfp, 4);

  if (fp < thisfp || fp > stack_end)
    {
      /* handle the Metaware-type pseudo-frame */

      func_start = get_pc_function_start(thisframe->pc);
      if (func_start)
	{
	  instr = read_memory_integer (func_start, 4);
	  /* Recognize "addu|adds -X,sp,sp" insn. */
	  if ((instr & 0xEFFF0000) == 0x84420000)
	      offset = SIGN_EXT16(instr & 0x0000FFFF);
	}

      fp = 0;
      if (offset < 0)
	fp = thisfp - offset;
    }
    return(fp);
}

frame_saved_pc(frame)
    unsigned long frame;
{
  unsigned long pc;
  unsigned long pc1;

  pc = read_memory_integer (frame + 4, 4);

  if (pc < text_start || pc > text_end)
    {
      /* handle the Metaware-type pseudo-frame */

      pc1 = read_memory_integer (frame, 4);
      if (pc1 >= text_start && pc1 <= text_end)
	pc = pc1;
      else
	pc = 0;
    }
  return(pc);
}

/* Pass arguments to a function in the inferior process - ABI compliant
 */

pass_function_arguments(args, nargs, struct_return)
    value *args;
    int nargs;
    int struct_return;
{
  int ireg = (struct_return) ? 17 : 16;
  int freg = FP0_REGNUM + 8;
  int i;
  struct type *type;
  value arg;
  long tmp;
  value value_arg_coerce();


  for (i = 0; i < nargs; i++) 
    {
      arg = value_arg_coerce(args[i]);
      type = VALUE_TYPE(arg);
      if (type == builtin_type_double) 
	{
	  write_register_bytes(REGISTER_BYTE(freg), VALUE_CONTENTS(arg), 8);
	  freg += 2;
	}
      else
	{
	  bcopy(VALUE_CONTENTS(arg), &tmp, 4);
	  write_register(ireg, tmp);
	  ireg++;
	}
    }
    if (ireg >= 28 || freg >= FP0_REGNUM + 16)
	error("Too many arguments to function");
}
