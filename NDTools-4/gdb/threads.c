/* Print and select threads for GDB, the GNU debugger, on Mach TT systems.
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

#ifdef MACH			/* MACH-only file */

#include <stdio.h>
#include <mach.h>

#include "defs.h"
#include "param.h"
#include "frame.h"
#include "inferior.h"
#include "symtab.h"

#ifdef ORC
#include <machine/reg.h>
#include <sys/param.h>
#include <sys/dir.h>
#include <sys/user.h>
#include <machine/thread_status.h>
#else ORC
#include "machine/reg.h"
#include "sys/param.h"
#include "sys/dir.h"
#include "sys/user.h"
#include "machine/thread_status.h"
#endif ORC

#ifdef	MACH_EXC
#ifdef	vax
#include <vax/psl.h>
#endif	vax
#ifdef i386
#include <machine/psl.h>
#endif i386
#endif	MACH_EXC

/* for this file only, we use the new_sun_ptrace code.  The other files need
   To belive that new_sun_ptrace==0 so that it will use the new TT code. */

#ifdef	sun3
#include <sun/ptrace.h>
#else	sun3
#include <sys/ptrace.h>
#endif	sun3

/*
#ifdef	CTHREADS
#include <cthreads.h>
#define	MTHREAD
#include <cthread_internals.h>
#endif	CTHREADS
*/

#include <cthreads.h>
#define	MTHREAD

#ifdef CTHREADS

#include <cthread_internals.h>

void		free_cprocs();
struct cproc	*get_cprocs();
cthread_t	selected_thread;



static char
translate_state(state)
int	state;
{
  switch (state) {
  case TH_STATE_RUNNING:	return('R');
  case TH_STATE_STOPPED:	return('S');
  case TH_STATE_WAITING:	return('W');
  case TH_STATE_UNINTERRUPTIBLE: return('U');
  case TH_STATE_HALTED:		return('H');
  default:			return('?');
  }
}

static char buf[5];

static char *
get_thread_name(th, n, cprocs)
thread_t th;
int n;
cproc_t cprocs;
{
    while (cprocs != NO_CPROC) {
	if ((char*)(cprocs->id) == (char *)th) {
	    if (cprocs->incarnation != NULL) {
		return (cprocs->incarnation->name);
	    } else {
		sprintf(buf, "_t%d", n);
	    }
	}
	cprocs = cprocs->link;
    }
    sprintf(buf, "_t%d", n);
}


void
thread_list_command()
{
    int i, current_thread_index;
    thread_array_t thread_table;
    int thread_count;
    thread_basic_info_data_t ths;
    cproc_t cprocs, my_cproc;
    char *name;
    int size;
    kern_return_t ret;

    ERROR_NO_INFERIOR;

    ret = task_threads(inferior_task, &thread_table, &thread_count);
    if (ret != KERN_SUCCESS) {
	fprintf(stderr, "Error getting inferior's thread list.\n");
	mach_error("task_threads", ret);
	return;
    }
    printf("There are %d threads.\n", thread_count);
    printf("Thread\tName\tState\tSuspend\tPort\n");
    cprocs = get_cprocs();
    for (i = 0; i < thread_count; i++) {
	if (thread_table[i] == current_thread)
	    current_thread_index = i;

	size = THREAD_BASIC_INFO_COUNT;
	ret = thread_info(thread_table[i], THREAD_BASIC_INFO,
			  (thread_info_t)&ths, &size);
	if (ret != KERN_SUCCESS) {
	    fprintf(stderr, "Unable to get info on thread 0x%x.\n",
		    thread_table[i]);
	    mach_error("thread_statistics", ret);
	    continue;
	}

	name = get_thread_name(thread_table[i], i, cprocs);

	printf ("%d\t%s\t%c\t%d\t0x%x\n", i, name,
		translate_state (ths.run_state), ths.suspend_count,
		thread_table[i]);
    }

    printf("\nThe current thread is thread %d.\n", current_thread_index);
    free_cprocs (cprocs);

    ret = vm_deallocate(task_self(), (vm_address_t)thread_table, 
			(thread_count * sizeof(int)));
    if (ret != KERN_SUCCESS) {
	fprintf(stderr, "Error trying to deallocate thread list.\n");
	mach_error("vm_deallocate", ret);
    }
}


void
free_cprocs( cprocs )
register cproc_t cprocs;
{
    register cproc_t my_cproc;

    while (cprocs != NO_CPROC) {
	if (cprocs->incarnation != NULL) {
	    if (cprocs->incarnation->name != NULL)
		free (cprocs->incarnation->name);
	    free (cprocs->incarnation);
	}
	my_cproc = cprocs->link;
	free (cprocs);
	cprocs = my_cproc;
    }
}


void
thread_select_command(args, from_tty)
     char *args;
     int from_tty;
{
  int thread_index;
  thread_array_t thread_list;
  int thread_count;
  kern_return_t ret;

  ERROR_NO_INFERIOR;

  if (!args)
    error_no_arg ("thread to select");

  thread_index = atoi(args);
  if (thread_index < 0) {
    error ("no such thread");
    return;
  }
  ret = task_threads (inferior_task, &thread_list, &thread_count);
  if (ret != KERN_SUCCESS) {
      mach_error ("task_threads", ret);
      exit (1);
  }
  if (thread_index > thread_count) {
    error ("no such thread");
    return;
  }

  current_thread = thread_list[thread_index];
  fetch_inferior_registers ();
  stop_pc = read_pc ();
  set_current_frame (read_register (FP_REGNUM), 1);

  ret = vm_deallocate(task_self(), (vm_address_t)thread_list, 
		      (thread_count * sizeof(int)));
  if (ret != KERN_SUCCESS) {
    fprintf(stderr, "Error trying to deallocate thread list.\n");
    mach_error("vm_deallocate", ret);
  }
}


cproc_t
get_cprocs()
{
    register struct symtab *table;
    register struct block  *bl;
    register struct symbol *sym;
    register int bl_i, sym_i, table_i;
    cproc_t their_cprocs, curr_cproc, cproc_copy, cproc_head;
    char *name;
    cthread_t cthread;

    their_cprocs = NO_CPROC;
    table = symtab_list;
    table_i = 0;
    while (table != NULL) {
	if (strcmp(table->filename, "cprocs.c") == 0) {
	    for (bl_i=0;bl_i<table->blockvector->nblocks; bl_i++){
		bl = table->blockvector->block[bl_i];
		for (sym_i = 0; sym_i < bl->nsyms; sym_i++) {
		    sym = bl->sym[sym_i];
		    if (strcmp(sym->name, "cprocs") == 0) {
			read_inferior_memory(sym->value.value, &their_cprocs,
					     sizeof(cproc_t));
			if (their_cprocs == NO_CPROC)
			    return( NO_CPROC );
			cproc_head = (cproc_t)malloc(sizeof(struct cproc));
			read_inferior_memory(their_cprocs, cproc_head,
					     sizeof(struct cproc));
			if (cproc_head->incarnation != NULL) {
			    cthread = (cthread_t)
					malloc(sizeof(struct cthread));
			    read_inferior_memory(cproc_head->incarnation,
						 cthread,
						 sizeof(struct cthread));
			    cproc_head->incarnation = cthread;
			    name = malloc(50);
			    read_inferior_memory(cthread->name, name, 50);
			    cthread->name = name;
			    break;
			}
		    }
		}
	    }
	}
	table = table->next; table_i++;
    }

    if (their_cprocs == NO_CPROC)
	return (NO_CPROC);

    curr_cproc = cproc_head;
    while (curr_cproc->link != NO_CPROC) {
	cproc_copy = (struct cproc *)malloc(sizeof(struct cproc));
	read_inferior_memory(curr_cproc->link, cproc_copy,
			     sizeof(struct cproc));
	/* add to tail, order is important */
	curr_cproc->link = cproc_copy;
	curr_cproc = curr_cproc->link;
	if (curr_cproc->incarnation != NULL) {
	    cthread = (cthread_t)malloc(sizeof(struct cthread));
	    read_inferior_memory(curr_cproc->incarnation, cthread,
				 sizeof(struct cthread));
	    curr_cproc->incarnation = cthread;
	    name = malloc(50);
	    read_inferior_memory(cthread->name, name, 50);
	    cthread->name = name;
	}
    }
    return(cproc_head);
}
#endif CTHREADS

#ifdef	MACH_EXC
set_trace_bit (thread)
{
#ifdef	sun
#define	flavor	SUN_THREAD_STATE_REGS
  unsigned int	stateCnt = SUN_THREAD_STATE_REGS_COUNT;
#endif	sun
#ifdef	romp
#define	flavor	ROMP_THREAD_STATE
  unsigned int	stateCnt = ROMP_THREAD_STATE_COUNT;
#endif	romp
#ifdef	vax
#define	flavor	VAX_THREAD_STATE
  unsigned int	stateCnt = VAX_THREAD_STATE_COUNT;
#endif	vax
#ifdef i386
#define flavor  i386_THREAD_STATE
  unsigned int  stateCnt = i386_THREAD_STATE_COUNT;
#endif i386
  kern_return_t		ret;
  thread_state_data_t	state;
  int bla;

  ret = thread_get_state(thread, flavor, state, &stateCnt);
  if (ret != KERN_SUCCESS) {
    fprintf(stderr, "set_trace_bit: error reading status register\n");
    mach_error("thread_get_state", ret);
    exit (1);
  }

#ifdef	sun
  ((struct sun_thread_state *)state)->sr =
    ((struct sun_thread_state *)state)->sr | 0x8000;
  bla = ((struct sun_thread_state *)state)->sr;
#endif	sun
#ifdef	vax
  ((struct vax_thread_state *)state)->ps =
    ((struct vax_thread_state *)state)->ps | PSL_T;
#endif	vax
#ifdef	romp
  fprintf(stderr, "fix threads.c: set_trace_bit() for the romp\n");
#endif	romp
#ifdef i386
  /* hell, I don't know what I'm doing but anyway... */
  (( struct i386_thread_state *)state)->efl |= EFL_TF;
#endif i386

  ret = thread_set_state(thread, flavor, state, stateCnt);
  if (ret != KERN_SUCCESS) {
    fprintf(stderr, "set_trace_bit: error writing status register\n");
    mach_error("thread_set_state", ret);
    exit (1);
  }  
  ret = thread_get_state(thread, flavor, state, &stateCnt);
  if (ret != KERN_SUCCESS) {
    fprintf(stderr, "set_trace_bit: error reading status register\n");
    mach_error("thread_get_state", ret);
    exit (1);
  }
}

clear_trace_bit (thread)
{
#ifdef	sun
#define	flavor	SUN_THREAD_STATE_REGS
  unsigned int	stateCnt = SUN_THREAD_STATE_REGS_COUNT;
#endif	sun
#ifdef	romp
#define	flavor	ROMP_THREAD_STATE
  unsigned int	stateCnt = ROMP_THREAD_STATE_COUNT;
#endif	romp
#ifdef	vax
#define	flavor	VAX_THREAD_STATE
  unsigned int	stateCnt = VAX_THREAD_STATE_COUNT;
#endif	vax
#ifdef i386
#define flavor  i386_THREAD_STATE
  unsigned int  stateCnt = i386_THREAD_STATE_COUNT;
#endif i386
  kern_return_t		ret;
  thread_state_data_t	state;

  ret = thread_get_state(thread, flavor, state, &stateCnt);
  if (ret != KERN_SUCCESS) {
    fprintf(stderr, "clear_trace_bit: error reading status register\n");
    mach_error("thread_get_state", ret);
    exit (1);
  }

#ifdef	sun
  ((struct sun_thread_state *)state)->sr = 
	((struct sun_thread_state *)state)->sr & 0x7fff;
#endif	sun
#ifdef	vax
  ((struct vax_thread_state *)state)->ps =
    ((struct vax_thread_state *)state)->ps & ~PSL_T;
#endif	vax
#ifdef	romp
  fprintf(stderr, "fix threads.c: clear_trace_bit() for the romp\n");
  exit (2);
#endif	romp
#ifdef i386
  (( struct i386_thread_state *)state)->efl &= ~EFL_TF;
#endif i386

  ret = thread_set_state(thread, flavor, state, stateCnt);
  if (ret != KERN_SUCCESS) {
    fprintf(stderr, "set_trace_bit: error writing status register\n");
    mach_error("thread_set_state", ret);
    exit (1);
  }  
}



suspend_command (args, from_tty)
     char *args;
     int from_tty;
{
  int thread_index;
  thread_array_t thread_list;
  int thread_count;
  kern_return_t ret;

  if (!args)
    error_no_arg ("thread to suspend or ALL");

  if (strcmp(args, "all") == 0) {
    suspend_all_threads();
    printf("WARNING: you have just suspended the thread that ptrace is\n");
    printf("using.  Type \"info ptrace\" for details.");
    return;
  }

  thread_index = atoi(args);
  if (thread_index < 0) {
    error ("no such thread");
    return;
  }

  ret = task_threads (inferior_task, &thread_list, &thread_count);
  if (ret != KERN_SUCCESS) {
      mach_error ("task_threads", ret);
      exit (1);
  }

  if (thread_index > thread_count) {
    error ("no such thread");
    return;
  }

  ret = thread_suspend (thread_list[thread_index]);
  if (ret != KERN_SUCCESS) {
      mach_error ("thread_suspend", ret);
  }

  if (thread_list[thread_index] == current_thread) {
    printf("WARNING: you have just suspended the thread that ptrace is\n");
    printf("using.  Type \"info ptrace\" for details.");
  }

  ret = vm_deallocate(task_self(), (vm_address_t)thread_list, 
		      (thread_count * sizeof(int)));
  if (ret != KERN_SUCCESS) {
    fprintf(stderr, "Error trying to deallocate thread list.\n");
    mach_error("vm_deallocate", ret);
  }
}

suspend_all_threads ()
{
    thread_array_t	thread_list;
    int			thread_count;
    kern_return_t	ret;

    ret = task_threads(inferior_task, &thread_list, &thread_count);
    if (ret != KERN_SUCCESS) {
	fprintf("Error trying to suspend all threads.\n");
	mach_error("task_threads", ret);
	exit (1);
    }
    while (thread_count > 0) {
	ret = thread_suspend(thread_list[--thread_count]);
	if (ret != KERN_SUCCESS) {
	    fprintf("Error trying to suspend thread 0x%x.\n", 
		    thread_list[--thread_count]);
	    mach_error("thread_suspend", ret);
	}
    }
    ret = vm_deallocate(task_self(), (vm_address_t)thread_list, 
			(thread_count * sizeof(int)));
    if (ret != KERN_SUCCESS) {
	fprintf(stderr, "Error trying to deallocate thread list.\n");
	mach_error("vm_deallocate", ret);
    }
}

resume_command (args, from_tty)
     char *args;
     int from_tty;
{
  int thread_index;
  thread_array_t thread_list;
  int thread_count;
  kern_return_t ret;

  if (!args)
    error_no_arg ("thread to resume or ALL");

  if (strcmp(args, "all") == 0) {
    resume_all_threads();
    return;
  }

  thread_index = atoi(args);
  if (thread_index < 0) {
    error ("no such thread");
    return;
  }

  ret = task_threads (inferior_task, &thread_list, &thread_count);
  if (ret != KERN_SUCCESS) {
      mach_error ("task_threads", ret);
      exit (1);
  }

  if (thread_index > thread_count) {
    error ("no such thread");
    return;
  }

  ret = thread_resume (thread_list[thread_index]);
  if (ret != KERN_SUCCESS) {
      mach_error ("thread_resume", ret);
  }

  ret = vm_deallocate(task_self(), (vm_address_t)thread_list, 
		      (thread_count * sizeof(int)));
  if (ret != KERN_SUCCESS) {
    fprintf(stderr, "Error trying to deallocate thread list.\n");
    mach_error("vm_deallocate", ret);
  }
}


resume_all_threads ()
{
    thread_array_t	thread_list;
    int			thread_count;
    kern_return_t	ret;

    ret = task_threads(inferior_task, &thread_list, &thread_count);
    if (ret != KERN_SUCCESS) {
	fprintf(stderr, "Error trying to resume all threads.\n");
	mach_error("task_threads", ret);
	exit (1);
    }
    while (thread_count > 0) {
	ret = thread_resume(thread_list[--thread_count]);
	if (ret != KERN_SUCCESS) {
	    fprintf("Error trying to resume thread 0x%x.\n", 
		    thread_list[--thread_count]);
	    mach_error("thread_resume", ret);
	    exit (1);
	}
    }
    ret = vm_deallocate(task_self(), (vm_address_t)thread_list, 
			(thread_count * sizeof(int)));
    if (ret != KERN_SUCCESS) {
	fprintf(stderr, "Error trying to deallocate thread list.\n");
	mach_error("vm_deallocate", ret);
    }
}
#else	MACH_EXC

/*
 *  The point here is to figure out which thread ptrace() is talking to.
 *  ptrace() is the only thing that knows which thread returned us to the
 *  debugger, so we use it to get the SP.  Once we have that, we look thru
 *  all of the threads to see which one has the same SP.  Then we set
 *  current_thread so the world knows what's going on.  Someday, we'll have
 *  the mach debugging facilities, but until then, we have to grovel.
 */
thread_t
compute_current_thread()
{
  int desired_stack_pointer;
  register int i;
  thread_array_t thread_list;
  int thread_count;
  thread_state_data_t state;
  kern_return_t ret;
  boolean_t resumed;
  struct user u;
  register unsigned int offset; /* = (char *) &u.u_ar0 - (char *) &u; */
#ifdef  sun
#define FLAVOR  SUN_THREAD_STATE_REGS
  unsigned int  stateCnt = SUN_THREAD_STATE_REGS_COUNT;
#endif  sun
#ifdef  vax
#define FLAVOR  VAX_THREAD_STATE
  unsigned int  stateCnt = VAX_THREAD_STATE_COUNT;
#endif  vax
#ifdef  ibm032
#define FLAVOR  ROMP_THREAD_STATE
  unsigned int	stateCnt = ROMP_THREAD_STATE_COUNT;
#endif  ibm032
#ifdef i386
#define FLAVOR  i386_THREAD_STATE
  unsigned int  stateCnt = i386_THREAD_STATE_COUNT;
#endif i386

#ifdef	sun3
  struct regs inferior_registers;

  /* Boy, this sun_ptrace stuff works NICE! */
  ptrace (PTRACE_GETREGS, inferior_pid, &inferior_registers);
  desired_stack_pointer = inferior_registers.r_sp;
#else	sun3

#ifdef	ibm032
#define USTRUCT (UPAGES*NBPG - sizeof(struct user))
#if	ROMP_APC
#define Reg0 (USTRUCT - 34*sizeof(int))
#else	ROMP_APC
#define Reg0 (USTRUCT - 18*sizeof(int))
#endif	ROMP_APC
#define Reg1 (Reg0 + sizeof(int))
  offset = Reg1;
#endif	ibm032

#ifdef	vax
  offset = ctob(UPAGES) - 5 * sizeof(int);
#endif	vax
#ifdef i386
  offset = ctob( UPAGES) - SAVED_NUM_REGS * 4;
  offset += UESP * 4;
#endif i386

    desired_stack_pointer = ptrace (PT_READ_U, inferior_pid, offset, 0);
#endif	sun3


  ret = task_threads (inferior_task, &thread_list, &thread_count);
  if (ret != KERN_SUCCESS) {
    mach_error ("task_threads", ret);
    exit (1);
  }
  /* loop thru the threads looking for the one using this stack */
  for (i = 0; i < thread_count; i++) {
    ret = thread_get_state (thread_list[i], FLAVOR, state, &stateCnt);
    if (ret != KERN_SUCCESS) {
      mach_error ("thread_get_state", ret);
      exit (1);
    }
#ifdef	sun3
    if (desired_stack_pointer == ((struct sun_thread_state *)state)->sp)
#endif	sun3
#ifdef	ibm032
    if (desired_stack_pointer == ((struct romp_thread_state *)state)->r1)
#endif	ibm032
#ifdef	vax
    if (desired_stack_pointer == ((struct vax_thread_state *)state)->sp)
#endif	vax
#ifdef i386
    if ( desired_stack_pointer == ((struct i386_thread_state *)state)->uesp)
#endif i386
      break;
  }
  if (i >= thread_count) {
    fprintf (stderr, "Could not determine which thread ptrace is using.\n");
    fflush (stderr);
    exit (-2);
  }
  free (state);
  current_thread = thread_list[i];
  ret = vm_deallocate (task_self(), thread_list, (thread_count * sizeof(int)));
  if (ret != KERN_SUCCESS) {
    fprintf(stderr, "Error deallocating thread list.\n");
    mach_error("vm_deallocate", ret);
  }
}
#endif	!MACH_EXC

#ifdef	MACH_EXC
ptrace_info ()
{
  printf(
   "ptrace is the unix function which all debuggers use to manipulate their\ninferiors.  It is implemented in such a way that if a debugger calls ptrace on\na thread which is suspended, or which is in a task which is suspended, ptrace\nwill block.  All subsequent calls to ptrace made by any other debugger on the\nsystem will also block.  In other words, if you don't resume that thread\nbefore continuing the inferior, you could inhibit everyone from using\ndebuggers on this machine until it is rebooted.");
}
#endif	MACH_EXC


void
_initialize_threads ()
{
  add_com ("thread-select", class_stack, thread_select_command,
	   "Select and print a thread.");
  add_com_alias ("ts", "thread-select", class_stack, 0);

  add_com ("thread-list", class_stack, thread_list_command,
	   "List all of threads.");
  add_com_alias ("tl", "thread-list", class_stack, 0);

#ifdef	MACH_EXC
  add_com ("suspend", class_run, suspend_command,
	   "Suspend one or all of the threads in the inferior.");
  add_com ("resume", class_run, resume_command,
	   "Resume one or all of the threads in the inferior.");
  add_info ("ptrace", ptrace_info, "Warning about suspend and ptrace.");
#endif	MACH_EXC
}

#endif MACH
