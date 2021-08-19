/* Start and stop the inferior process, for GDB.
   Copyright (C) 1986, 1987, 1988 Free Software Foundation, Inc.
*/

/*
 $Header: /notos/gdb-3.0/dist-gdb/RCS/infrun.c,v 1.5 89/05/10 12:35:11 binding Exp $

 $Log:	infrun.c,v $
 * Revision 1.5  89/05/10  12:35:11  binding
 * added more mach exceptions to be dealt with: EXC_BAD_INSTRUCTION and
 * EXC_BAD_ACCESS
 * 
 * Revision 1.4  89/04/28  15:03:11  binding
 * added support for graceful interaction between Mach debug exceptions
 * and UNIX signals
 * also changed back the <kern/mach.h> include to <mach.h>
 * 
 * Revision 1.3  89/04/14  22:15:22  kupfer
 * <mach.h> -> <kern/mach.h> (I may regret this...)
 * 
 * Revision 1.2  89/03/13  15:23:14  binding
 * fixed comparison in catch_exception_raise: if ( other_threads_suspended...
 * 
*/

/*
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

/* Notes on the algorithm used in wait_for_inferior to determine if we
   just did a subroutine call when stepping.  We have the following
   information at that point:

                  Current and previous (just before this step) pc.
		  Current and previous sp.
		  Current and previous start of current function.

   If the start's of the functions don't match, then

   	a) We did a subroutine call.

   In this case, the pc will be at the beginning of a function.

	b) We did a subroutine return.

   Otherwise.

	c) We did a longjmp.

   If we did a longjump, we were doing "nexti", since a next would
   have attempted to skip over the assembly language routine in which
   the longjmp is coded and would have simply been the equivalent of a
   continue.  I consider this ok behaivior.  We'd like one of two
   things to happen if we are doing a nexti through the longjmp()
   routine: 1) It behaves as a stepi, or 2) It acts like a continue as
   above.  Given that this is a special case, and that anybody who
   thinks that the concept of sub calls is meaningful in the context
   of a longjmp, I'll take either one.  Let's see what happens.  

   Acts like a subroutine return.  I can handle that with no problem
   at all.

   -->So: If the current and previous beginnings of the current
   function don't match, *and* the pc is at the start of a function,
   we've done a subroutine call.  If the pc is not at the start of a
   function, we *didn't* do a subroutine call.  

   -->If the beginnings of the current and previous function do match,
   either: 

   	a) We just did a recursive call.

	   In this case, we would be at the very beginning of a
	   function and 1) it will have a prologue (don't jump to
	   before prologue, or 2) (we assume here that it doesn't have
	   a prologue) there will have been a change in the stack
	   pointer over the last instruction.  (Ie. it's got to put
	   the saved pc somewhere.  The stack is the usual place.  In
	   a recursive call a register is only an option if there's a
	   prologue to do something with it.  This is even true on
	   register window machines; the prologue sets up the new
	   window.  It might not be true on a register window machine
	   where the call instruction moved the register window
	   itself.  Hmmm.  One would hope that the stack pointer would
	   also change.  If it doesn't, somebody send me a note, and
	   I'll work out a more general theory.
	   randy@wheaties.ai.mit.edu).  This is true (albeit slipperly
	   so) on all machines I'm aware of:

	      m68k:	Call changes stack pointer.  Regular jumps don't.

	      sparc:	Recursive calls must have frames and therefor,
	                prologues.

	      vax:	All calls have frames and hence change the
	                stack pointer.

	b) We did a return from a recursive call.  I don't see that we
	   have either the ability or the need to distinguish this
	   from an ordinary jump.  The stack frame will be printed
	   when and if the frame pointer changes; if we are in a
	   function without a frame pointer, it's the users own
	   lookout.

	c) We did a jump within a function.  We assume that this is
	   true if we didn't do a recursive call.

	d) We are in no-man's land ("I see no symbols here").  We
	   don't worry about this; it will make calls look like simple
	   jumps (and the stack frames will be printed when the frame
	   pointer moves), which is a reasonably non-violent response.

#ifdef SUN4_COMPILER_FEATURE
    We do a special ifdef for the sun 4, forcing it to single step
  into calls which don't have prologues.  This means that we can't
  nexti over leaf nodes, we can probably next over them (since they
  won't have debugging symbols, usually), and we can next out of
  functions returning structures (with a "call .stret4" at the end).
#endif
*/

#include "defs.h"
#include "param.h"
#include "symtab.h"
#include "frame.h"
#include "inferior.h"
#include "wait.h"

#include <stdio.h>
#include <signal.h>
#include <a.out.h>
/* unistd.h is needed to #define X_OK */
#ifdef USG
#include <unistd.h>
#else
#include <sys/file.h>
#endif

#ifdef MACH
#include <mach.h>
#include <mach_exception.h>
#include <sys/exception.h>
#include <machine/exception.h>
#include <sys/message.h>
#endif MACH


#ifdef UMAX_PTRACE
#include <sys/param.h>
#include <sys/ptrace.h>
#endif /* UMAX_PTRACE */

extern char *sys_siglist[];
extern int errno;

/* Tables of how to react to signals; the user sets them.  */

static char signal_stop[NSIG];
static char signal_print[NSIG];
static char signal_program[NSIG];

/* Nonzero if breakpoints are now inserted in the inferior.  */

static int breakpoints_inserted;

/* Function inferior was in as of last step command.  */

static struct symbol *step_start_function;

/* This is the sequence of bytes we insert for a breakpoint.  */

static char break_insn[] = BREAKPOINT;

/* Nonzero => address for special breakpoint for resuming stepping.  */

static CORE_ADDR step_resume_break_address;

/* Original contents of the byte where the special breakpoint is.  */

static char step_resume_break_shadow[sizeof break_insn];

/* Nonzero means the special breakpoint is a duplicate
   so it has not itself been inserted.  */

static int step_resume_break_duplicate;

/* Nonzero if we are expecting a trace trap and should proceed from it.
   2 means expecting 2 trace traps and should continue both times.
   That occurs when we tell sh to exec the program: we will get
   a trap after the exec of sh and a second when the program is exec'd.  */

static int trap_expected;

/* Nonzero if the next time we try to continue the inferior, it will
   step one instruction and generate a spurious trace trap.
   This is used to compensate for a bug in HP-UX.  */

static int trap_expected_after_continue;

/* Nonzero means expecting a trace trap
   and should stop the inferior and return silently when it happens.  */

int stop_after_trap;

/* Nonzero means expecting a trace trap due to attaching to a process.  */

int stop_after_attach;

/* Nonzero if pc has been changed by the debugger
   since the inferior stopped.  */

int pc_changed;

/* Nonzero if debugging a remote machine via a serial link or ethernet.  */

int remote_debugging;

/* Save register contents here when about to pop a stack dummy frame.  */

char stop_registers[REGISTER_BYTES];

/* Nonzero if program stopped due to error trying to insert breakpoints.  */

static int breakpoints_failed;

/* Nonzero if inferior is in sh before our program got exec'd.  */

static int running_in_shell;

/* Nonzero after stop if current stack frame should be printed.  */

static int stop_print_frame;

#ifdef NO_SINGLE_STEP
extern int one_stepped;		/* From machine dependent code */
extern void single_step ();	/* Same. */
#endif /* NO_SINGLE_STEP */

static void insert_step_breakpoint ();
static void remove_step_breakpoint ();
static void wait_for_inferior ();
static void normal_stop ();


/* Clear out all variables saying what to do when inferior is continued.
   First do this, then set the ones you want, then call `proceed'.  */

void
clear_proceed_status ()
{
  trap_expected = 0;
  step_range_start = 0;
  step_range_end = 0;
  step_frame_address = 0;
  step_over_calls = -1;
  step_resume_break_address = 0;
  stop_after_trap = 0;
  stop_after_attach = 0;

  /* Discard any remaining commands left by breakpoint we had stopped at.  */
  clear_breakpoint_commands ();
}

/* Basic routine for continuing the program in various fashions.

   ADDR is the address to resume at, or -1 for resume where stopped.
   SIGNAL is the signal to give it, or 0 for none,
     or -1 for act according to how it stopped.
   STEP is nonzero if should trap after one instruction.
     -1 means return after that and print nothing.
     You should probably set various step_... variables
     before calling here, if you are stepping.

   You should call clear_proceed_status before calling proceed.  */

void
proceed (addr, signal, step)
     CORE_ADDR addr;
     int signal;
     int step;
{
  int oneproc = 0;

  if (step > 0)
    step_start_function = find_pc_function (read_pc ());
  if (step < 0)
    stop_after_trap = 1;

  if (addr == -1)
    {
      /* If there is a breakpoint at the address we will resume at,
	 step one instruction before inserting breakpoints
	 so that we do not stop right away.  */

      if (!pc_changed && breakpoint_here_p (read_pc ()))
	oneproc = 1;
    }
  else
    {
      write_register (PC_REGNUM, addr);
#ifdef NPC_REGNUM
      write_register (NPC_REGNUM, addr + 4);
#endif
    }

  if (trap_expected_after_continue)
    {
      /* If (step == 0), a trap will be automatically generated after
	 the first instruction is executed.  Force step one
	 instruction to clear this condition.  This should not occur
	 if step is nonzero, but it is harmless in that case.  */
      oneproc = 1;
      trap_expected_after_continue = 0;
    }

  if (oneproc)
    /* We will get a trace trap after one instruction.
       Continue it automatically and insert breakpoints then.  */
    trap_expected = 1;
  else
    {
      int temp = insert_breakpoints ();
      if (temp)
	{
	  print_sys_errmsg ("ptrace", temp);
	  error ("Cannot insert breakpoints.\n\
The same program may be running in another process.");
	}
      breakpoints_inserted = 1;
    }

  /* Install inferior's terminal modes.  */
  terminal_inferior ();

  if (signal >= 0) stop_signal = signal;
  /* If this signal should not be seen by program,
     give it zero.  Used for debugging signals.  */
  else if (stop_signal < NSIG && !signal_program[stop_signal]) stop_signal= 0;

  /* Resume inferior.  */
#ifdef DEBUG
  fprintf( stderr, "infrun: proceed: calling resume (%d, %d)\n",
	   oneproc || step, stop_signal);
#endif DEBUG

  resume (oneproc || step, stop_signal);

  /* Wait for it to stop (if not standalone)
     and in any case decode why it stopped, and act accordingly.  */

#ifdef DEBUG
  fprintf( stderr, "infrun: proceed: waitint for inferior\n");
#endif DEBUG

  wait_for_inferior (); normal_stop ();
}

/* Writing the inferior pc as a register calls this function
   to inform infrun that the pc has been set in the debugger.  */

void
writing_pc (val)
     CORE_ADDR val;
{
  stop_pc = val;
  pc_changed = 1;
}

/* Start an inferior process for the first time.
   Actually it was started by the fork that created it,
   but it will have stopped one instruction after execing sh.
   Here we must get it up to actual execution of the real program.  */

void
start_inferior ()
{
  /* We will get a trace trap after one instruction.
     Continue it automatically.  Eventually (after shell does an exec)
     it will get another trace trap.  Then insert breakpoints and continue.  */

#ifdef START_INFERIOR_TRAPS_EXPECTED
  trap_expected = START_INFERIOR_TRAPS_EXPECTED;
#else
  trap_expected = 2;
#endif

  running_in_shell = 0;		/* Set to 1 at first SIGTRAP, 0 at second.  */
  trap_expected_after_continue = 0;
  breakpoints_inserted = 0;
  mark_breakpoints_out ();

  /* Set up the "saved terminal modes" of the inferior
     based on what modes we are starting it with.  */
  terminal_init_inferior ();

  /* Install inferior's terminal modes.  */
  terminal_inferior ();

  if (remote_debugging)
    {
      trap_expected = 0;
      fetch_inferior_registers();
      set_current_frame (create_new_frame (read_register (FP_REGNUM),
					   read_pc ()));
      stop_frame_address = FRAME_FP (get_current_frame());
      inferior_pid = 3;
      if (insert_breakpoints())
	fatal("Can't insert breakpoints");
      breakpoints_inserted = 1;
      proceed(-1, -1, 0);
    }
  else { wait_for_inferior (); normal_stop ();}
}

#ifdef	MACH_EXC

/* in case of MACH we need to record exceptions on break points and traces */
/* because wait3() does *not* reliably return the state of the debuggee  */
/* in the case when a breakpoint is set on a signal handling routine */
/* in fact, when hitting a breakpoint coincidentally with a signal  */
/* wait3() would return pid == 0, w == 0 and thus pretend the debuggee no */
/* longer exists! */

typedef struct {
  int exception, code, subcode;
} mach_exception_status;

static mach_exception_status exc_status;

#ifdef i386

#define MACH_BREAK( x) ( (x).exception == EXC_BREAKPOINT && \
		 ((x).code == EXC_I386_BPT || (x).code == EXC_I386_SGL))
#define MACH_ILL_INSTR( x) ( (x).exception == EXC_BAD_INSTRUCTION && \
			     (x).code == EXC_I386_INVOP)

#define MACH_SEGV( x) ( (x).exception == EXC_BAD_ACCESS)

#define mach_arch_exception mach_i386_exception
#endif i386




/* this stuff here is an upcall via libmach/excServer.c */
/* and wait_for_inferior (infrun.c) which does the actual upcall */

catch_exception_raise (port, thread, task, exception, code, subcode)
port_t port;
thread_t thread;
task_t task;
int exception, code, subcode;
{
  kern_return_t ret;

#ifdef DEBUG
  mach_arch_exception( "infrun: catch_exc_raise", exception, code, subcode);
#endif DEBUG
  exc_status.exception = exception; exc_status.code = code; 
  exc_status.subcode = subcode;
  current_thread = thread;
  if ((ret = task_suspend (task)) != KERN_SUCCESS) {
    fprintf (stderr, "Error suspending victim after exception.\n");
    mach_error ("task_suspend", ret);
    exit (1);
  }
  if (other_threads_suspended == TRUE) {
    if ((ret = thread_suspend (current_thread)) != KERN_SUCCESS) {
      mach_error ("thread_suspend", ret);
      exit (1);
    }
    clear_trace_bit (current_thread);
    resume_all_threads ();
    other_threads_suspended = FALSE;
  }
#ifdef DEBUG
  fprintf( stderr, "infrun: leaving catch_exc_raise\n");
#endif DEBUG
  return(KERN_SUCCESS);
}

static int Exc2Sig( sig, exc)
mach_exception_status exc;
/* maps a Mach exception to a UNIX signal */
/* notice that sig may already been set by UNIX in which case we leave */
/* it untouched */
{
  if ( MACH_BREAK( exc)) return( SIGTRAP);
  else if ( MACH_SEGV( exc)) return( SIGSEGV);
  else if ( MACH_ILL_INSTR( exc)) return( SIGILL);
  else return( sig);
}

#else   MACH_EXC

#define MACH_BREAK( x) FALSE
#define MACH_SEGV( x) FALSE

typedef char mach_exception_status;
static mach_exception_status exc_status = 0;

#endif	MACH_EXC

/* Start remote-debugging of a machine over a serial link.  */

void
start_remote ()
{
  clear_proceed_status ();
  running_in_shell = 0;
  trap_expected = 0;
  inferior_pid = 3;
  breakpoints_inserted = 0;
  mark_breakpoints_out ();
  wait_for_inferior ();
  normal_stop();
}

#ifdef ATTACH_DETACH

/* Attach to process PID, then initialize for debugging it
   and wait for the trace-trap that results from attaching.  */

void
attach_program (pid)
     int pid;
{
  attach (pid);
  inferior_pid = pid;

  mark_breakpoints_out ();
  terminal_init_inferior ();
  clear_proceed_status ();
  stop_after_attach = 1;
  /*proceed (-1, 0, -2);*/
  wait_for_inferior ();
  normal_stop ();
}
#endif /* ATTACH_DETACH */


static inferior_terminated( exception, w) 
mach_exception_status exception;
WAITTYPE w;
/* if inferior exited, it's time to clean it up.... */
{ char mach_exc = MACH_BREAK( exception) || MACH_SEGV( exception);

  if ( !mach_exc && WIFEXITED (w)) {
    terminal_ours_for_output ();
    if (WRETCODE (w))
      printf ("\nProgram exited with code 0%o.\n", WRETCODE (w));
    else
      printf ("\nProgram exited normally.\n");
    fflush (stdout);
    inferior_died ();
    stop_print_frame = 0;
    return( TRUE);
  } else return( FALSE);
}

static inferior_not_stopped( exception, w)
mach_exception_status exception;
WAITTYPE w;
/* if the debuggee isn't exited, and isn't stopped (via a signal or bkpt) */
/* we assume a "serious" signal to kill the debuggee ... */
{ char mach_break = MACH_BREAK( exception);

  if ( !mach_break && !WIFSTOPPED (w)) {
    kill_inferior ();
    stop_print_frame = 0;
    stop_signal = WTERMSIG (w);
#ifdef MACH_EXC
    stop_signal = Exc2Sig( stop_signal, exception);
#endif MACH_EXC
    terminal_ours_for_output ();
    printf("\nProgram terminated with signal %d, %s\n", stop_signal,
	   stop_signal<NSIG ? sys_siglist[stop_signal]:"(undocumented)");
    printf ("The inferior process no longer exists.\n");
    fflush (stdout);
    return( TRUE);
  }
  return( FALSE);
}

/* Wait for control to return from inferior to debugger.
   If inferior gets a signal, we may decide to start it up again
   instead of returning.  That is why there is a loop in this function.
   When this function actually returns it means the inferior
   should be left stopped and GDB should read more commands.  */

static void
wait_for_inferior ()
{
  register int pid;
  WAITTYPE w;
  CORE_ADDR pc;
  int tem;
  int another_trap;
  int random_signal;
  CORE_ADDR stop_sp, prev_sp;
  CORE_ADDR prev_func_start, stop_func_start;
  CORE_ADDR prologue_pc;
  int stop_step_resume_break;
  CORE_ADDR step_resume_break_sp;
  int newmisc;
  int newfun_pc;
  struct symbol *newfun;
  struct symtab_and_line sal;
  int prev_pc;
  extern CORE_ADDR text_end;
#ifdef	MACH_EXC
  kern_return_t ret;
  unsigned int tiCnt;
  boolean_t exception_occured = FALSE;

  struct msg {
    msg_header_t    header;
    int             data[20];
  } in_msg, out_msg;
#endif	MACH_EXC
#ifdef CALL_DUMMY_VARIABLE
  extern CORE_ADDR call_dummy_set;	/* true if dummy call being done */
  extern CORE_ADDR call_dummy_start;	/* address of call dummy code */
#endif /* CALL_DUMMY_VARIABLE */

  prev_pc = read_pc ();
  prev_func_start = get_pc_function_start (prev_pc) + FUNCTION_START_OFFSET;
  prev_sp = read_register (SP_REGNUM);

  while (1)
    {
      if (remote_debugging) remote_wait (&w);
      else {
#ifdef	MACH_EXC
	/* Here's where we do the waiting part */
	stopped_in_ptrace = FALSE;
	exc_status.exception = exc_status.code = -1;
	while (TRUE) { /* look and see if it sent us an exception */
	  in_msg.header.msg_local_port = inferior_exception_port; 
	  in_msg.header.msg_size = sizeof(in_msg); 
	  ret = msg_receive (&in_msg.header, RCV_TIMEOUT, 500); 
	  if (ret == RCV_SUCCESS) { /* the server will suspend the task ... */
	    exc_server (&in_msg.header, &out_msg.header);
	    /* he'll try and run, but he's suspended,  */
	    /* so all he'll do is be ready to leave the rpc */
	    msg_send (&out_msg.header, MSG_OPTION_NONE, 0);
	    stopped_in_ptrace = FALSE;
	    break;  
	  } /* if ret == RCV_SUCCESS ... */
#ifdef DEBUG
	     else mach_error( "infrun: wait_for_inferior", ret); 
#endif DEBUG
	  /* Notice that on the first SIGTRAPs sent from execve we don't */
          /* get a msg on execption port. Thus the wait3 is still necessary */
          /* only breakpoint traps come through exception port .... */
	  /* look and see if it stopped */
	  pid = wait3 (&w, WNOHANG, 0);  /* do that to set value of w */
#ifdef DEBUG
	  fprintf( stderr, "wait3( w = %o (%d), pid = %d)\n", w, w, pid);
#endif DEBUG
	  if (pid != 0) {
	    stopped_in_ptrace = TRUE;
	    break;
	  } /* if pid !=0 .. */
	} /* while ( TRUE) ... */
#else	MACH_EXC
	pid = wait (&w);
	if (pid != inferior_pid) {
	  fprintf( stderr, "wait_for_inferior: pid != inferior_pid\n");
	  continue;
	}
#endif MACH_EXC
      }

#ifdef MACH
      /* need that here to avoid the fetch_inferior_registers on a dead */
      /* process below... */
      if ( inferior_terminated( exc_status, w))
	break;
#endif MACH

#ifdef NO_SINGLE_STEP
      if (one_stepped) single_step (0); /* This actually cleans up the ss */
#endif /* NO_SINGLE_STEP */

      pc_changed = 0; flush_cached_frames ();
      fetch_inferior_registers (); stop_pc = read_pc ();
      set_current_frame ( create_new_frame (read_register (FP_REGNUM),
					    read_pc ()));
#ifdef CONVEX_PTRACE
      /* pop frame stored by user-mode trap, if present */
      if (stop_pc == BREAK_TRAP_ADDR)
	{
	  POP_FRAME;
	  stop_pc = read_pc () - 2;
	  write_register (PC_REGNUM, stop_pc);
#ifdef NPC_REGNUM
	  write_register (NPC_REGNUM, stop_pc + 4);
#endif
	  pc_changed = 0;
	}
      else if (stop_pc > STACK_END_ADDR)
	{
	  POP_FRAME;
	  stop_pc = read_pc ();
	}
#endif /* CONVEX_PTRACE */
      stop_frame_address = FRAME_FP (get_current_frame ());
      stop_sp = read_register (SP_REGNUM);
      stop_func_start =
	get_pc_function_start (stop_pc) + FUNCTION_START_OFFSET;
      another_trap = stop_breakpoint = stop_step = stop_stack_dummy = 0;
      stop_step_resume_break = random_signal = stopped_by_random_signal = 0;
      breakpoints_failed = 0;
      stop_print_frame = 1;

      /* Look at the cause of the stop, and decide what to do.
	 The alternatives are:
	 1) break; to really stop and return to the debugger,
	 2) drop through to start up again
	   (set another_trap to 1 to single step once)
	 3) set random_signal to 1, and the decision between 1 and 2
	   will be made according to the signal handling tables.  */
#ifndef MACH
      /* this could possibly be migrated to the MACH case ? */
      if ( inferior_terminated( exc_status, w))
	break;
#endif MACH
      if ( inferior_not_stopped( exc_status, w))
	break;
      else
	{

	  stop_signal = WSTOPSIG (w);
#ifdef MACH_EXC
	  /* Mach no longer sets the process status properly when */
	  /* treating an exception */
	  stop_signal = Exc2Sig( stop_signal, exc_status);
#endif MACH_EXC

#ifdef DEBUG
	  fprintf( stderr, "infrun: stop_signal = %d\n", stop_signal);
#endif DEBUG

	  /* First, distinguish signals caused by the debugger from signals
	     that have to do with the program's own actions.
	     Note that breakpoint insns may cause SIGTRAP or SIGILL
	     or SIGEMT, depending on the operating system version.
	     Here we detect when a SIGILL or SIGEMT is really a breakpoint
	     and change it to SIGTRAP.  */

	  if (stop_signal == SIGTRAP 
#ifndef CONVEX_PTRACE
	      || (breakpoints_inserted && 
		  (stop_signal == SIGILL || stop_signal == SIGEMT))
#endif /* not CONVEX_PTRACE */
	      || stop_after_attach)
	    {
	      if (stop_signal == SIGTRAP && stop_after_trap) {
		stop_print_frame = 0; break;
	      }
	      if (stop_after_attach) break;
	      /* Don't even think about breakpoints
		 if still running the shell that will exec the program
		 or if just proceeded over a breakpoint.  */
	      if (stop_signal == SIGTRAP && trap_expected) stop_breakpoint = 0;
	      else
		/* See if there is a breakpoint at the current PC.  */
#if DECR_PC_AFTER_BREAK
		/* Notice the case of stepping through a jump
		   that leads just after a breakpoint.
		   Don't confuse that with hitting the breakpoint.
		   What we check for is that 1) stepping is going on
		   and 2) the pc before the last insn does not match
		   the address of the breakpoint before the current pc.  */
		if (!(prev_pc != stop_pc - DECR_PC_AFTER_BREAK
		      && step_range_end && !step_resume_break_address))
#endif /* DECR_PC_AFTER_BREAK not zero */
		  {
		    /* For condition exprs. */
		    select_frame (get_current_frame (), 0);
		    stop_breakpoint = breakpoint_stop_status (stop_pc, stop_frame_address);
		    /* Following in case break condition called a function.  */
		    stop_print_frame = 1;
		    if (stop_breakpoint && DECR_PC_AFTER_BREAK)
		      {
			stop_pc -= DECR_PC_AFTER_BREAK;
			write_register (PC_REGNUM, stop_pc);
#ifdef NPC_REGNUM
			write_register (NPC_REGNUM, stop_pc + 4);
#endif
			pc_changed = 0;
		      }
		  }
	      /* See if we stopped at the special breakpoint for
		 stepping over a subroutine call.  */
	      if (stop_pc - DECR_PC_AFTER_BREAK == step_resume_break_address)
		{
		  stop_step_resume_break = 1;
		  if (DECR_PC_AFTER_BREAK)
		    {
		      stop_pc -= DECR_PC_AFTER_BREAK;
		      write_register (PC_REGNUM, stop_pc);
		      pc_changed = 0;
		    }
		}

	      if (stop_signal == SIGTRAP)
		random_signal
		  = !(stop_breakpoint || trap_expected
		      || stop_step_resume_break
#ifdef CALL_DUMMY_VARIABLE
		      || (call_dummy_set && stop_pc >= call_dummy_start 
			  && stop_pc <= call_dummy_start + CALL_DUMMY_LENGTH)
#else
#ifndef CONVEX_PTRACE
		      || (stop_sp INNER_THAN stop_pc
			  && stop_pc INNER_THAN stop_frame_address)
#else
		      || stop_pc == text_end - 2
#endif
#endif	/* CALL_DUMMY_VARIABLE */
		      || (step_range_end && !step_resume_break_address));
	      else
		{
		  random_signal
		    = !(stop_breakpoint
			|| stop_step_resume_break
#ifdef news800
			|| (stop_sp INNER_THAN stop_pc
			    && stop_pc INNER_THAN stop_frame_address)
#endif

			);
		  if (!random_signal)
		    stop_signal = SIGTRAP;
		}
	    }
	  else
	    random_signal = 1;

	  /* For the program's own signals, act according to
	     the signal handling tables.  */

	  if (random_signal
	      && !(running_in_shell && stop_signal == SIGSEGV))
	    {
	      /* Signal not for debugging purposes.  */
	      int printed = 0;

	      stopped_by_random_signal = 1;

	      if (stop_signal >= NSIG
		  || signal_print[stop_signal])
		{
		  printed = 1;
		  terminal_ours_for_output ();
		  printf ("\nProgram received signal %d, %s\n",
			  stop_signal,
			  stop_signal < NSIG
			   ? sys_siglist[stop_signal]
			   : "(undocumented)");
		  fflush (stdout);
		}
	      if (stop_signal >= NSIG
		  || signal_stop[stop_signal])
		break;
	      /* If not going to stop, give terminal back
		 if we took it away.  */
	      else if (printed)
		terminal_inferior ();
	    }

	  /* Handle cases caused by hitting a breakpoint.  */

	  if (!random_signal
	      && (stop_breakpoint || stop_step_resume_break))
	    {
	      /* Does a breakpoint want us to stop?  */
	      if (stop_breakpoint && stop_breakpoint != -1
		  && stop_breakpoint != -0x1000001)
		{
		  /* 0x1000000 is set in stop_breakpoint as returned by
		     breakpoint_stop_status to indicate a silent
		     breakpoint.  */
		  if ((stop_breakpoint > 0 ? stop_breakpoint :
		       -stop_breakpoint)
		      & 0x1000000)
		    {
		      stop_print_frame = 0;
		      if (stop_breakpoint > 0)
			stop_breakpoint -= 0x1000000;
		      else
			stop_breakpoint += 0x1000000;
		    }
		  break;
		}
	      /* But if we have hit the step-resumption breakpoint,
		 remove it.  It has done its job getting us here.
		 The sp test is to make sure that we don't get hung
		 up in recursive calls in functions without frame
		 pointers.  If the stack pointer isn't outside of
		 where the breakpoint was set (within a routine to be
		 stepped over), we're in the middle of a recursive
		 call. Not true for reg window machines (sparc)
		 because the must change frames to call things and
		 the stack pointer doesn't have to change if it
		 the bp was set in a routine without a frame (pc can
		 be stored in some other window).  

		 Also the i860 call instruction puts the return address in
		 a register and does not change the stack pointer, so we
		 must skip the sp test - JGH. */

	      if (stop_step_resume_break
		  && (step_frame_address == 0
		      || (stop_frame_address == step_frame_address
#if !defined(HAVE_REGISTER_WINDOWS) && !defined(RETURN_ADDR_IN_REGISTER)
			  && step_resume_break_sp INNER_THAN stop_sp
#endif
			  )))
		{
		  remove_step_breakpoint ();
		  step_resume_break_address = 0;
		}
	      /* Otherwise, must remove breakpoints and single-step
		 to get us past the one we hit.  */
	      else
		{
		  remove_breakpoints ();
		  remove_step_breakpoint ();
		  breakpoints_inserted = 0;
		  another_trap = 1;
		}

	      /* We come here if we hit a breakpoint but should not
		 stop for it.  Possibly we also were stepping
		 and should stop for that.  So fall through and
		 test for stepping.  But, if not stepping,
		 do not stop.  */
	    }

	  /* If this is the breakpoint at the end of a stack dummy,
	     just stop silently.  */
#ifdef CALL_DUMMY_VARIABLE
	  if (call_dummy_set && stop_pc >= call_dummy_start 
	      && stop_pc <= call_dummy_start + CALL_DUMMY_LENGTH)
#else
#ifndef CONVEX_PTRACE
	  if (stop_sp INNER_THAN stop_pc
	      && stop_pc INNER_THAN stop_frame_address)
#else
	      /* "stack" dummy must be in text segment for Convex Unix */
	  if (stop_pc == text_end - 2)
#endif
#endif	/* CALL_DUMMY_VARIABLE */
	    {
	      stop_print_frame = 0;
	      stop_stack_dummy = 1;
#ifdef HP9K320
	      trap_expected_after_continue = 1;
#endif
	      break;
	    }

	  if (step_resume_break_address)
	    /* Having a step-resume breakpoint overrides anything
	       else having to do with stepping commands until
	       that breakpoint is reached.  */
	    ;
	  /* If stepping through a line, keep going if still within it.  */
	  else if (!random_signal
		   && step_range_end
		   && stop_pc >= step_range_start
		   && stop_pc < step_range_end
		   /* The step range might include the start of the
		      function, so if we are at the start of the
		      step range and either the stack or frame pointers
		      just changed, we've stepped outside */
		   && !(stop_pc == step_range_start
			&& stop_frame_address
			&& (stop_sp != prev_sp
			    || stop_frame_address != step_frame_address)))
	    {
	      /* Don't step through the return from a function
		 unless that is the first instruction stepped through.  */
	      if (ABOUT_TO_RETURN (stop_pc))
		{
		  stop_step = 1;
		  break;
		}
	    }

	  /* We stepped out of the stepping range.  See if that was due
	     to a subroutine call that we should proceed to the end of.  */
	  else if (!random_signal && step_range_end)
	    {
	      if (stop_func_start)
		{
		  prologue_pc = stop_func_start;
		  SKIP_PROLOGUE (prologue_pc);
		}

	      /* ==> See comments at top of file on this algorithm.  <==*/

	      if (stop_pc == stop_func_start
		  && (stop_func_start != prev_func_start
		      || prologue_pc != stop_func_start
		      || stop_sp != prev_sp)
#ifdef SUN4_COMPILER_FEATURE
		  && prologue_pc != stop_func_start
#endif
		  )
		{
		  newfun = find_pc_function (stop_pc);
		  /* It's a subroutine call */
		  if (step_over_calls > 0 || (step_over_calls && newfun == 0))
		    {
		      /* A subroutine call has happened.  */
		      /* Set a special breakpoint after the return */
		      step_resume_break_address =
			SAVED_PC_AFTER_CALL (stop_frame_address);
		      step_resume_break_duplicate
			= breakpoint_here_p (step_resume_break_address);
		      step_resume_break_sp = stop_sp;
		      if (breakpoints_inserted)	insert_step_breakpoint ();
		    }
		  /* Subroutine call with source code we should not step over.
		     Do step to the first line of code in it.  */
		  else if (step_over_calls)
		    {
		      SKIP_PROLOGUE (stop_func_start);
		      sal = find_pc_line (stop_func_start, 0);
		      /* Use the step_resume_break to step until
			 the end of the prologue, even if that involves jumps
			 (as it seems to on the vax under 4.2).  */
		      /* If the prologue ends in the middle of a source line,
			 continue to the end of that source line.
			 Otherwise, just go to end of prologue.  */
#ifdef convex
		      /* no, don't either.  It skips any code that's
			 legitimately on the first line.  */
#else
		      if (sal.end && sal.pc != stop_func_start)
			stop_func_start = sal.end;
#endif
		      
		      if (stop_func_start == stop_pc)
			{
			  /* We are already there: stop now.  */
			  stop_step = 1;
			  break;
			}
		      else
			/* Put the step-breakpoint there and go until there. */
			{
			  step_resume_break_address = stop_func_start;
			  step_resume_break_sp = stop_sp;
			  
			  step_resume_break_duplicate
			    = breakpoint_here_p (step_resume_break_address);
			  if (breakpoints_inserted)
			    insert_step_breakpoint ();
			  /* Do not specify what the fp should be when we stop
			     since on some machines the prologue
			     is where the new fp value is established.  */
			  step_frame_address = 0;
			  /* And make sure stepping stops right away then.  */
			  step_range_end = step_range_start;
			}
		    }
		  else
		    {
		      /* We get here only if step_over_calls is 0 and we
			 just stepped into a subroutine.  I presume
			 that step_over_calls is only 0 when we're
			 supposed to be stepping at the assembly
			 language level.*/
		      stop_step = 1;
		      break;
		    }
		}
	      /* No subroutince call; stop now.  */
	      else
		{
		  stop_step = 1;
		  break;
		}
	    }
	}

      /* Save the pc before execution, to compare with pc after stop.  */
      prev_pc = read_pc ();	/* Might have been DECR_AFTER_BREAK */
      prev_func_start = stop_func_start; /* Ok, since if DECR_PC_AFTER
					  BREAK is defined, the
					  original pc would not have
					  been at the start of a
					  function. */
      prev_sp = stop_sp;

      /* If we did not do break;, it means we should keep
	 running the inferior and not return to debugger.  */

      /* If trap_expected is 2, it means continue once more
	 and insert breakpoints at the next trap.
	 If trap_expected is 1 and the signal was SIGSEGV, it means
	 the shell is doing some memory allocation--just resume it
	 with SIGSEGV.
	 Otherwise insert breakpoints now, and possibly single step.  */

      if (trap_expected > 1)
	{
	  trap_expected--;
	  running_in_shell = 1;
	  resume (0, 0);
	}
      else if (running_in_shell && stop_signal == SIGSEGV)
	{
	  resume (0, SIGSEGV);
	}
      else
	{
	  /* Here, we are not awaiting another exec to get
	     the program we really want to debug.
	     Insert breakpoints now, unless we are trying
	     to one-proceed past a breakpoint.  */
	  running_in_shell = 0;
	  if (!breakpoints_inserted && !another_trap)
	    {
	      insert_step_breakpoint ();
	      breakpoints_failed = insert_breakpoints ();
	      if (breakpoints_failed)
		break;
	      breakpoints_inserted = 1;
	    }

	  trap_expected = another_trap;

	  if (stop_signal == SIGTRAP) stop_signal = 0;

	  resume ( (step_range_end && !step_resume_break_address)
		   || trap_expected, stop_signal);
	}
    }
}

/* Here to return control to GDB when the inferior stops for real.
   Print appropriate messages, remove breakpoints, give terminal our modes.

   RUNNING_IN_SHELL nonzero means the shell got a signal before
   exec'ing the program we wanted to run.
   STOP_PRINT_FRAME nonzero means print the executing frame
   (pc, function, args, file, line number and line text).
   BREAKPOINTS_FAILED nonzero means stop was due to error
   attempting to insert breakpoints.  */

static void
normal_stop ()
{ 

#ifndef MACH
  /* Make sure that the current_frame's pc is correct.  This
     is a correction for setting up the frame info before doing
     DECR_PC_AFTER_BREAK */
  (get_current_frame())->pc = read_pc ();
#endif !MACH

  if (breakpoints_failed)
    {
      terminal_ours_for_output ();
      print_sys_errmsg ("ptrace", breakpoints_failed);
      printf ("Stopped; cannot insert breakpoints.\n\
The same program may be running in another process.\n");
    }

  if (inferior_pid)
    remove_step_breakpoint ();

  if (inferior_pid && breakpoints_inserted)
    if (remove_breakpoints ())
      {
	terminal_ours_for_output ();
	printf ("Cannot remove breakpoints because program is no longer writable.\n\
It must be running in another process.\n\
Further execution is probably impossible.\n");
      }

  breakpoints_inserted = 0;

  /* Delete the breakpoint we stopped at, if it wants to be deleted.
     Delete any breakpoint that is to be deleted at the next stop.  */

  breakpoint_auto_delete (stop_breakpoint);

  /* If an auto-display called a function and that got a signal,
     delete that auto-display to avoid an infinite recursion.  */

  if (stopped_by_random_signal) delete_current_display ();

  if (step_multi && stop_step) return;

  terminal_ours ();

  if (running_in_shell)
    {
      if (stop_signal == SIGSEGV)
	{
	  char *exec_file = (char *) get_exec_file (1);

	  if (access (exec_file, X_OK) != 0)
	    printf ("The file \"%s\" is not executable.\n", exec_file);
	  else
	    printf ("\
You have just encountered a bug in \"sh\".  GDB starts your program\n\
by running \"sh\" with a command to exec your program.\n\
This is so that \"sh\" will process wildcards and I/O redirection.\n\
This time, \"sh\" crashed.\n\
\n\
One known bug in \"sh\" bites when the environment takes up a lot of space.\n\
Try \"info env\" to see the environment; then use \"unset-env\" to kill\n\
some variables whose values are large; then do \"run\" again.\n\
\n\
If that works, you might want to put those \"unset-env\" commands\n\
into a \".gdbinit\" file in this directory so they will happen every time.\n");
	}
      /* Don't confuse user with his program's symbols on sh's data.  */
      stop_print_frame = 0;
    }

  if (inferior_pid == 0) return;

#ifdef MACH
  /* Make sure that the current_frame's pc is correct.  This
     is a correction for setting up the frame info before doing
     DECR_PC_AFTER_BREAK */
  (get_current_frame())->pc = read_pc ();
#endif MACH

  /* Select innermost stack frame except on return from a stack dummy routine,
     or if the program has exited.  */
  if (!stop_stack_dummy)
    {
      select_frame (get_current_frame (), 0);

      if (stop_print_frame)
	{
	  if (stop_breakpoint > 0)
	    printf ("\nBpt %d, ", stop_breakpoint);
	  print_sel_frame (stop_step
			   && step_frame_address == stop_frame_address
			   && step_start_function == find_pc_function (stop_pc));
	  /* Display the auto-display expressions.  */
	  do_displays ();
	}
    }

  /* Save the function value return registers
     We might be about to restore their previous contents.  */
  read_register_bytes (0, stop_registers, REGISTER_BYTES);

  if (stop_stack_dummy)
    {
      /* Pop the empty frame that contains the stack dummy.
         POP_FRAME ends with a setting of the current frame, so we
	 can use that next. */
      POP_FRAME;
      select_frame (get_current_frame (), 0);
    }
}

static void
insert_step_breakpoint ()
{
  if (step_resume_break_address && !step_resume_break_duplicate)
    {
      read_memory (step_resume_break_address,
		   step_resume_break_shadow, sizeof break_insn);
      write_memory (step_resume_break_address,
		    break_insn, sizeof break_insn);
    }
}

static void
remove_step_breakpoint ()
{
  if (step_resume_break_address && !step_resume_break_duplicate)
    write_memory (step_resume_break_address, step_resume_break_shadow,
		  sizeof break_insn);
}

/* Specify how various signals in the inferior should be handled.  */

static void
handle_command (args, from_tty)
     char *args;
     int from_tty;
{
  register char *p = args;
  int signum;
  register int digits, wordlen;

  if (!args)
    error_no_arg ("signal to handle");

  while (*p)
    {
      /* Find the end of the next word in the args.  */
      for (wordlen = 0; p[wordlen] && p[wordlen] != ' ' && p[wordlen] != '\t';
	   wordlen++);
      for (digits = 0; p[digits] >= '0' && p[digits] <= '9'; digits++);

      /* If it is all digits, it is signal number to operate on.  */
      if (digits == wordlen)
	{
	  signum = atoi (p);
	  if (signum == SIGTRAP || signum == SIGINT)
	    {
	      if (!query ("Signal %d is used by the debugger.\nAre you sure you want to change it? ", signum))
		error ("Not confirmed.");
	    }
	}
      else if (signum == 0)
	error ("First argument is not a signal number.");

      /* Else, if already got a signal number, look for flag words
	 saying what to do for it.  */
      else if (!strncmp (p, "stop", wordlen))
	{
	  signal_stop[signum] = 1;
	  signal_print[signum] = 1;
	}
      else if (wordlen >= 2 && !strncmp (p, "print", wordlen))
	signal_print[signum] = 1;
      else if (wordlen >= 2 && !strncmp (p, "pass", wordlen))
	signal_program[signum] = 1;
      else if (!strncmp (p, "ignore", wordlen))
	signal_program[signum] = 0;
      else if (wordlen >= 3 && !strncmp (p, "nostop", wordlen))
	signal_stop[signum] = 0;
      else if (wordlen >= 4 && !strncmp (p, "noprint", wordlen))
	{
	  signal_print[signum] = 0;
	  signal_stop[signum] = 0;
	}
      else if (wordlen >= 4 && !strncmp (p, "nopass", wordlen))
	signal_program[signum] = 0;
      else if (wordlen >= 3 && !strncmp (p, "noignore", wordlen))
	signal_program[signum] = 1;
      /* Not a number and not a recognized flag word => complain.  */
      else
	{
	  p[wordlen] = 0;
	  error ("Unrecognized flag word: \"%s\".", p);
	}

      /* Find start of next word.  */
      p += wordlen;
      while (*p == ' ' || *p == '\t') p++;
    }

  if (from_tty)
    {
      /* Show the results.  */
      printf ("Number\tStop\tPrint\tPass to program\tDescription\n");
      printf ("%d\t", signum);
      printf ("%s\t", signal_stop[signum] ? "Yes" : "No");
      printf ("%s\t", signal_print[signum] ? "Yes" : "No");
      printf ("%s\t\t", signal_program[signum] ? "Yes" : "No");
      printf ("%s\n", sys_siglist[signum]);
    }
}

/* Print current contents of the tables set by the handle command.  */

static void
signals_info (signum_exp)
     char *signum_exp;
{
  register int i;
  printf ("Number\tStop\tPrint\tPass to program\tDescription\n");

  if (signum_exp)
    {
      i = parse_and_eval_address (signum_exp);
      printf ("%d\t", i);
      printf ("%s\t", signal_stop[i] ? "Yes" : "No");
      printf ("%s\t", signal_print[i] ? "Yes" : "No");
      printf ("%s\t\t", signal_program[i] ? "Yes" : "No");
      printf ("%s\n", sys_siglist[i]);
      return;
    }

  printf ("\n");
  for (i = 0; i < NSIG; i++)
    {
      QUIT;
      if (i > 0 && i % 16 == 0)
	{
	  printf ("[Type Return to see more]");
	  fflush (stdout);
	  read_line ();
	}
      printf ("%d\t", i);
      printf ("%s\t", signal_stop[i] ? "Yes" : "No");
      printf ("%s\t", signal_print[i] ? "Yes" : "No");
      printf ("%s\t\t", signal_program[i] ? "Yes" : "No");
      printf ("%s\n", sys_siglist[i]);
    }

  printf ("\nUse the \"handle\" command to change these tables.\n");
}

/* Save all of the information associated with the inferior<==>gdb
   connection.  INF_STATUS is a pointer to a "struct inferior_status"
   (defined in inferior.h).  */

struct command_line *get_breakpoint_commands ();

void
save_inferior_status (inf_status, restore_stack_info)
     struct inferior_status *inf_status;
     int restore_stack_info;
{
  inf_status->pc_changed = pc_changed;
  inf_status->stop_signal = stop_signal;
  inf_status->stop_pc = stop_pc;
  inf_status->stop_frame_address = stop_frame_address;
  inf_status->stop_breakpoint = stop_breakpoint;
  inf_status->stop_step = stop_step;
  inf_status->stop_stack_dummy = stop_stack_dummy;
  inf_status->stopped_by_random_signal = stopped_by_random_signal;
  inf_status->trap_expected = trap_expected;
  inf_status->step_range_start = step_range_start;
  inf_status->step_range_end = step_range_end;
  inf_status->step_frame_address = step_frame_address;
  inf_status->step_over_calls = step_over_calls;
  inf_status->step_resume_break_address = step_resume_break_address;
  inf_status->stop_after_trap = stop_after_trap;
  inf_status->stop_after_attach = stop_after_attach;
  inf_status->breakpoint_commands = get_breakpoint_commands ();
  inf_status->restore_stack_info = restore_stack_info;
  
  bcopy (stop_registers, inf_status->stop_registers, REGISTER_BYTES);
  
  record_selected_frame (&(inf_status->selected_frame_address),
			 &(inf_status->selected_level));
  return;
}

void
restore_inferior_status (inf_status)
     struct inferior_status *inf_status;
{
  FRAME fid;
  int level = inf_status->selected_level;

  pc_changed = inf_status->pc_changed;
  stop_signal = inf_status->stop_signal;
  stop_pc = inf_status->stop_pc;
  stop_frame_address = inf_status->stop_frame_address;
  stop_breakpoint = inf_status->stop_breakpoint;
  stop_step = inf_status->stop_step;
  stop_stack_dummy = inf_status->stop_stack_dummy;
  stopped_by_random_signal = inf_status->stopped_by_random_signal;
  trap_expected = inf_status->trap_expected;
  step_range_start = inf_status->step_range_start;
  step_range_end = inf_status->step_range_end;
  step_frame_address = inf_status->step_frame_address;
  step_over_calls = inf_status->step_over_calls;
  step_resume_break_address = inf_status->step_resume_break_address;
  stop_after_trap = inf_status->stop_after_trap;
  stop_after_attach = inf_status->stop_after_attach;
  set_breakpoint_commands (inf_status->breakpoint_commands);

  bcopy (inf_status->stop_registers, stop_registers, REGISTER_BYTES);

  if (inf_status->restore_stack_info)
    {
      fid = find_relative_frame (get_current_frame (),
				 &level);
      
      if (FRAME_FP (fid) != inf_status->selected_frame_address ||
	  level != 0)
	{
	  fprintf (stderr,
		   "Call to inferior appears to have corrupted stack.\n");
	  select_frame (get_current_frame (), 0);
	  return;
	}
      
      select_frame (fid, inf_status->selected_level);
    }
  return;
}


void
_initialize_infrun ()
{
  register int i;

  add_info ("signals", signals_info,
	    "What debugger does when program gets various signals.\n\
Specify a signal number as argument to print info on that signal only.");

  add_com ("handle", class_run, handle_command,
	   "Specify how to handle a signal.\n\
Args are signal number followed by flags.\n\
Flags allowed are \"stop\", \"print\", \"pass\",\n\
 \"nostop\", \"noprint\" or \"nopass\".\n\
Print means print a message if this signal happens.\n\
Stop means reenter debugger if this signal happens (implies print).\n\
Pass means let program see this signal; otherwise program doesn't know.\n\
Pass and Stop may be combined.");

  for (i = 0; i < NSIG; i++)
    {
      signal_stop[i] = 1;
      signal_print[i] = 1;
      signal_program[i] = 1;
    }

  /* Signals caused by debugger's own actions
     should not be given to the program afterwards.  */
  signal_program[SIGTRAP] = 0;
  signal_program[SIGINT] = 0;

  /* Signals that are not errors should not normally enter the debugger.  */
#ifdef SIGALRM
  signal_stop[SIGALRM] = 0;
  signal_print[SIGALRM] = 0;
#endif /* SIGALRM */
#ifdef SIGVTALRM
  signal_stop[SIGVTALRM] = 0;
  signal_print[SIGVTALRM] = 0;
#endif /* SIGVTALRM */
#ifdef SIGPROF
  signal_stop[SIGPROF] = 0;
  signal_print[SIGPROF] = 0;
#endif /* SIGPROF */
#ifdef SIGCHLD
  signal_stop[SIGCHLD] = 0;
  signal_print[SIGCHLD] = 0;
#endif /* SIGCHLD */
#ifdef SIGCLD
  signal_stop[SIGCLD] = 0;
  signal_print[SIGCLD] = 0;
#endif /* SIGCLD */
#ifdef SIGIO
  signal_stop[SIGIO] = 0;
  signal_print[SIGIO] = 0;
#endif /* SIGIO */
#ifdef SIGURG
  signal_stop[SIGURG] = 0;
  signal_print[SIGURG] = 0;
#endif /* SIGURG */
}

