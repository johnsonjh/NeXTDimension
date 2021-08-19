/* Get info from stack frames;
   convert between frames, blocks, functions and pc values.
   Copyright (C) 1986, 1987, 1988 Free Software Foundation, Inc.

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

#include <stdio.h>

#include "defs.h"
#include "param.h"
#include "symtab.h"
#include "frame.h"

/* Address of end of first object file.
   This file is assumed to be a startup file
   and frames with pc's inside it
   are treated as nonexistent.  */

CORE_ADDR first_object_file_end;

/* Address of innermost stack frame (contents of FP register) */

static FRAME current_frame;

struct block *block_for_pc ();
CORE_ADDR get_pc_function_start ();

/*
 * Cache for frame addresses already read by gdb.  Valid only while
 * inferior is stopped.  Control variables for the frame cache should
 * be local to this module.
 */
struct obstack frame_cache_obstack;

/* Return the innermost (currently executing) stack frame.  */

FRAME
get_current_frame ()
{
  /* We assume its address is kept in a general register;
     param.h says which register.  */

  return current_frame;
}

void
set_current_frame (frame)
     FRAME frame;
{
  current_frame = frame;
}

FRAME
create_new_frame (addr, pc)
     FRAME_ADDR addr;
     CORE_ADDR pc;
{
  struct frame_info *fci;	/* Same type as FRAME */

  fci = (struct frame_info *)
    obstack_alloc (&frame_cache_obstack,
		   sizeof (struct frame_info));

  /* Arbitrary frame */
  fci->next = (struct frame_info *) 0;
  fci->prev = (struct frame_info *) 0;
  fci->frame = addr;
  fci->next_frame = 0;		/* Since arbitrary */
  fci->pc = pc;

#ifdef INIT_EXTRA_FRAME_INFO
  INIT_EXTRA_FRAME_INFO (fci);
#endif

  return fci;
}

/* Return the frame that called FRAME.
   If FRAME is the original frame (it has no caller), return 0.  */

FRAME
get_prev_frame (frame)
     FRAME frame;
{
  /* We're allowed to know that FRAME and "struct frame_info *" are
     the same */
  return get_prev_frame_info (frame);
}

/*
 * Flush the entire frame cache.
 */
void
flush_cached_frames ()
{
  /* Since we can't really be sure what the first object allocated was */
  obstack_free (&frame_cache_obstack, 0);
  obstack_init (&frame_cache_obstack);
  
  current_frame = (struct frame_info *) 0; /* Invalidate cache */
}

/* Return a structure containing various interesting information
   about a specified stack frame.  */
/* How do I justify including this function?  Well, the FRAME
   identifier format has gone through several changes recently, and
   it's not completely inconceivable that it could happen again.  If
   it does, have this routine around will help */

struct frame_info *
get_frame_info (frame)
     FRAME frame;
{
  return frame;
}

/* Return a structure containing various interesting information
   about the frame that called FRAME.

   This is much faster than get_frame_info (get_prev_frame (FRAME))
   because it does not need to search the entire stack
   to find the frame called by the one being described -- that is FRAME.  */

struct frame_info *
get_prev_frame_info (next_frame)
     FRAME next_frame;
{
  FRAME_ADDR address;
  struct frame_info *prev;
  int fromleaf = 0;

  /* If the requested entry is in the cache, return it.
     Otherwise, figure out what the address should be for the entry
     we're about to add to the cache. */

  if (!next_frame)
    {
      if (!current_frame)
	fatal ("get_prev_frame_info: Called before cache primed");

      return current_frame;
    }
  else
    {
      /* If we have the prev one, return it */
      if (next_frame->prev)
	return next_frame->prev;

      if (!(next_frame->next))
	{
	  /* Innermost */
	  CORE_ADDR func_start, after_prologue;

	  func_start = (get_pc_function_start (next_frame->pc) +
			FUNCTION_START_OFFSET);
	  after_prologue = func_start;
	  SKIP_PROLOGUE (after_prologue);
	  if (func_start == after_prologue)
	    {
	      fromleaf = 1;
	      address = next_frame->frame;
	    }
	}

      if (!fromleaf)
	{
	  /* Two macros defined in param.h specify the machine-dependent
	     actions to be performed here.  */
	  /* First, get the frame's chain-pointer.
	     If that is zero, the frame is the outermost frame.  */
#ifdef FRAME_CHAIN_POINTER
	  address = FRAME_CHAIN_POINTER (next_frame);
#else
	  address = FRAME_CHAIN (next_frame->frame);
#endif
	  if (!FRAME_CHAIN_VALID (address, next_frame->frame))
	    return 0;
	  /* If frame has a caller, combine the chain pointer and
	     the frame's own address to get the address of the caller.  */
	  address = FRAME_CHAIN_COMBINE (address, next_frame->frame_address);
	}
    }

  prev = (struct frame_info *)
    obstack_alloc (&frame_cache_obstack,
		   sizeof (struct frame_info));

  if (next_frame)
    next_frame->prev = prev;
  prev->next = next_frame;
  prev->prev = (struct frame_info *) 0;
  prev->frame = address;
  prev->next_frame = prev->next ? prev->next->frame : 0;

#ifdef INIT_EXTRA_FRAME_INFO
  INIT_EXTRA_FRAME_INFO(prev);
#endif

  /* This entry is in the frame queue now, which is good since
     FRAME_SAVED_PC may use that queue to figure out it's value
     (see m-sparc.h).  We want the pc saved in the inferior frame. */
  prev->pc = (fromleaf ? SAVED_PC_AFTER_CALL (next_frame->frame) :
	      next_frame ? FRAME_SAVED_PC (next_frame->frame) : read_pc ());

  return prev;
}

CORE_ADDR
get_frame_pc (frame)
     FRAME frame;
{
  struct frame_info *fi;
  fi = get_frame_info (frame);
  return fi->pc;
}

/* Find the addresses in which registers are saved in FRAME.  */

void
get_frame_saved_regs (frame_info_addr, saved_regs_addr)
     struct frame_info *frame_info_addr;
     struct frame_saved_regs *saved_regs_addr;
{
  FRAME_FIND_SAVED_REGS (frame_info_addr, *saved_regs_addr);
}

/* Return the innermost lexical block in execution
   in a specified stack frame.  The frame address is assumed valid.  */

struct block *
get_frame_block (frame)
     FRAME frame;
{
  struct frame_info *fi;

  fi = get_frame_info (frame);
  return block_for_pc (fi->pc);
}

struct block *
get_current_block ()
{
  return block_for_pc (read_pc ());
}

CORE_ADDR
get_pc_function_start (pc)
     CORE_ADDR pc;
{
  register struct block *bl = block_for_pc (pc);
  register struct symbol *symbol;
  if (bl == 0 || (symbol = block_function (bl)) == 0)
    {
      register int misc_index = find_pc_misc_function (pc);
      if (misc_index >= 0)
	return misc_function_vector[misc_index].address;
      return 0;
    }
  bl = SYMBOL_BLOCK_VALUE (symbol);
  return BLOCK_START (bl);
}

/* Return the symbol for the function executing in frame FRAME.  */

struct symbol *
get_frame_function (frame)
     FRAME frame;
{
  register struct block *bl = get_frame_block (frame);
  if (bl == 0)
    return 0;
  return block_function (bl);
}

/* Return the innermost lexical block containing the specified pc value,
   or 0 if there is none.  */

extern struct symtab *psymtab_to_symtab ();

struct block *
block_for_pc (pc)
     register CORE_ADDR pc;
{
  register struct block *b;
  register int bot, top, half;
  register struct symtab *s;
  register struct partial_symtab *ps;
  struct blockvector *bl;

  /* First search all symtabs for one whose file contains our pc */

  for (s = symtab_list; s; s = s->next)
    {
      bl = BLOCKVECTOR (s);
      b = BLOCKVECTOR_BLOCK (bl, 0);
      if (BLOCK_START (b) <= pc
	  && BLOCK_END (b) > pc)
	break;
    }

  if (s == 0)
    for (ps = partial_symtab_list; ps; ps = ps->next)
      {
	if (ps->textlow <= pc
	    && ps->texthigh > pc)
	  {
	    s = psymtab_to_symtab (ps);
	    bl = BLOCKVECTOR (s);
	    b = BLOCKVECTOR_BLOCK (bl, 0);
	    break;
	  }
      }

  if (s == 0)
    return 0;

  /* Then search that symtab for the smallest block that wins.  */
  /* Use binary search to find the last block that starts before PC.  */

  bot = 0;
  top = BLOCKVECTOR_NBLOCKS (bl);

  while (top - bot > 1)
    {
      half = (top - bot + 1) >> 1;
      b = BLOCKVECTOR_BLOCK (bl, bot + half);
      if (BLOCK_START (b) <= pc)
	bot += half;
      else
	top = bot + half;
    }

  /* Now search backward for a block that ends after PC.  */

  while (bot >= 0)
    {
      b = BLOCKVECTOR_BLOCK (bl, bot);
      if (BLOCK_END (b) > pc)
	return b;
      bot--;
    }

  return 0;
}

/* Return the function containing pc value PC.
   Returns 0 if function is not known.  */

struct symbol *
find_pc_function (pc)
     CORE_ADDR pc;
{
  register struct block *b = block_for_pc (pc);
  if (b == 0)
    return 0;
  return block_function (b);
}

/* Find the misc function whose address is the largest
   while being less than PC.  Return its index in misc_function_vector.
   Returns -1 if PC is not in suitable range.  */

int
find_pc_misc_function (pc)
     register CORE_ADDR pc;
{
  register int lo = 0;
  register int hi = misc_function_count-1;
  register int new;
  register int distance;

  /* Note that the last thing in the vector is always _etext.  */

  /* Above statement is not *always* true - fix for case where there are */
  /* no misc functions at all (ie no symbol table has been read). */
  if (hi < 0) return -1;        /* no misc functions recorded */

  /* trivial reject range test */
  if (pc < misc_function_vector[0].address ||
      pc > misc_function_vector[hi].address)
    return -1;

  do {
    new = (lo + hi) >> 1;
    distance = misc_function_vector[new].address - pc;
    if (distance == 0)
      return new;		/* an exact match */
    else if (distance > 0)
      hi = new;
    else
      lo = new;
  } while (hi-lo != 1);

  /* if here, we had no exact match, so return the lower choice */
  return lo;
}

/* Return the innermost stack frame executing inside of the specified block,
   or zero if there is no such frame.  */

FRAME
block_innermost_frame (block)
     struct block *block;
{
  struct frame_info *fi;
  register FRAME frame;
  register CORE_ADDR start = BLOCK_START (block);
  register CORE_ADDR end = BLOCK_END (block);

  frame = 0;
  while (1)
    {
      frame = get_prev_frame (frame);
      if (frame == 0)
	return 0;
      fi = get_frame_info (frame);
      if (fi->pc >= start && fi->pc < end)
	return frame;
    }
}

void
_initialize_blockframe ()
{
  obstack_init (&frame_cache_obstack);
}
