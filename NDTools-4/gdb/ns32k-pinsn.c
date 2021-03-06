/* Print 32000 instructions for GDB, the GNU debugger.
   Copyright (C) 1986,1988 Free Software Foundation, Inc.

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
#include "opcode.h"

/* 32000 instructions are never longer than this.  */
#define MAXLEN 62

/* Number of elements in the opcode table.  */
#define NOPCODES (sizeof notstrs / sizeof notstrs[0])

extern char *reg_names[];

#define NEXT_IS_ADDR	'|'

/*
 * extract "count" bits starting "offset" bits
 * into buffer
 */

int
bit_extract (buffer, offset, count)
     char *buffer;
     int offset;
     int count;
{
  int result;
  int mask;
  int bit;

  buffer += offset >> 3;
  offset &= 7;
  bit = 1;
  result = 0;
  while (count--) 
    {
      if ((*buffer & (1 << offset)))
	result |= bit;
      if (++offset == 8)
	{
	  offset = 0;
	  buffer++;
	}
      bit <<= 1;
    }
  return result;
}

float
fbit_extract (buffer, offset, count)
{
  union {
    int ival;
    float fval;
  } foo;

  foo.ival = bit_extract (buffer, offset, 32);
  return foo.fval;
}

double
dbit_extract (buffer, offset, count)
{
  union {
    struct {int low, high; } ival;
    double dval;
  } foo;

  foo.ival.low = bit_extract (buffer, offset, 32);
  foo.ival.high = bit_extract (buffer, offset+32, 32);
  return foo.dval;
}

sign_extend (value, bits)
{
  value = value & ((1 << bits) - 1);
  return (value & (1 << (bits-1))
	  ? value | (~((1 << bits) - 1))
	  : value);
}

flip_bytes (ptr, count)
     char *ptr;
     int count;
{
  char tmp;

  while (count > 0)
    {
      tmp = *ptr;
      ptr[0] = ptr[count-1];
      ptr[count-1] = tmp;
      ptr++;
      count -= 2;
    }
}


/* Print the 32000 instruction at address MEMADDR in debugged memory,
   on STREAM.  Returns length of the instruction, in bytes.  */

int
print_insn (memaddr, stream)
CORE_ADDR memaddr;
FILE *stream;
{
  unsigned char buffer[MAXLEN];
  register int i;
  register unsigned char *p;
  register char *d;
  unsigned short first_word;
  int gen, disp;
  int ioffset;		/* bits into instruction */
  int aoffset;		/* bits into arguments */
  char arg_bufs[MAX_ARGS+1][ARG_LEN];
  int argnum;
  int maxarg;

  read_memory (memaddr, buffer, MAXLEN);

  first_word = *(unsigned short *) buffer;
  for (i = 0; i < NOPCODES; i++)
    if ((first_word & ((1 << notstrs[i].detail.obits) - 1))
	== notstrs[i].detail.code)
      break;

  /* Handle undefined instructions.  */
  if (i == NOPCODES)
    {
      fprintf (stream, "0%o", buffer[0]);
      return 1;
    }

  fprintf (stream, "%s", notstrs[i].name);

  ioffset = notstrs[i].detail.ibits;
  aoffset = notstrs[i].detail.ibits;
  d = notstrs[i].detail.args;

  if (*d)
    {
      fputc ('\t', stream);

      maxarg = 0;
      while (*d)
	{
	  argnum = *d - '1';
	  d++;
	  if (argnum > maxarg && argnum < MAX_ARGS)
	    maxarg = argnum;
	  ioffset = print_insn_arg (*d, ioffset, &aoffset, buffer,
				    memaddr, arg_bufs[argnum]);
	  d++;
	}
      for (argnum = 0; argnum <= maxarg; argnum++)
	{
	  CORE_ADDR addr;
	  char *ch, *index ();
	  for (ch = arg_bufs[argnum]; *ch;)
	    {
	      if (*ch == NEXT_IS_ADDR)
		{
		  ++ch;
		  addr = atoi (ch);
		  print_address (addr, stream);
		  while (*ch && *ch != NEXT_IS_ADDR)
		    ++ch;
		  if (*ch)
		    ++ch;
		}
	      else
		putc (*ch++, stream);
	    }
	  if (argnum < maxarg)
	    fprintf (stream, ", ");
	}
    }
  return aoffset / 8;
}

print_insn_arg (d, ioffset, aoffsetp, buffer, addr, result)
     char d;
     int ioffset, *aoffsetp;
     char *buffer;
     CORE_ADDR addr;
     char *result;
{
  int addr_mode;
  float Fvalue;
  double Lvalue;
  int Ivalue;
  int disp1, disp2;
  int index;

  switch (d)
    {
    case 'F':
    case 'L':
    case 'B':
    case 'W':
    case 'D':
    case 'A':
      addr_mode = bit_extract (buffer, ioffset-5, 5);
      ioffset -= 5;
      switch (addr_mode)
	{
	case 0x0: case 0x1: case 0x2: case 0x3:
	case 0x4: case 0x5: case 0x6: case 0x7:
	  switch (d)
	    {
	    case 'F':
	    case 'L':
	      sprintf (result, "f%d", addr_mode);
	      break;
	    default:
	      sprintf (result, "r%d", addr_mode);
	    }
	  break;
	case 0x8: case 0x9: case 0xa: case 0xb:
	case 0xc: case 0xd: case 0xe: case 0xf:
	  disp1 = get_displacement (buffer, aoffsetp);
	  sprintf (result, "%d(r%d)", disp1, addr_mode & 7);
	  break;
	case 0x10:
	case 0x11:
	case 0x12:
	  disp1 = get_displacement (buffer, aoffsetp);
	  disp2 = get_displacement (buffer, aoffsetp);
	  sprintf (result, "%d(%d(%s))", disp2, disp1,
		   addr_mode==0x10?"fp":addr_mode==0x11?"sp":"sb");
	  break;
	case 0x13:
	  sprintf (result, "reserved");
	  break;
	case 0x14:
	  switch (d)
	    {
	    case 'B':
	      Ivalue = bit_extract (buffer, *aoffsetp, 8);
	      Ivalue = sign_extend (Ivalue, 8);
	      *aoffsetp += 8;
	      sprintf (result, "$%d", Ivalue);
	      break;
	    case 'W':
	      Ivalue = bit_extract (buffer, *aoffsetp, 16);
	      flip_bytes (&Ivalue, 2);
	      *aoffsetp += 16;
	      Ivalue = sign_extend (Ivalue, 16);
	      sprintf (result, "$%d", Ivalue);
	      break;
	    case 'D':
	      Ivalue = bit_extract (buffer, *aoffsetp, 32);
	      flip_bytes (&Ivalue, 4);
	      *aoffsetp += 32;
	      sprintf (result, "$%d", Ivalue);
	      break;
	    case 'A':
	      Ivalue = bit_extract (buffer, *aoffsetp, 32);
	      flip_bytes (&Ivalue, 4);
	      *aoffsetp += 32;
	      sprintf (result, "$|%d|", Ivalue);
	      break;
	    case 'F':
	      Fvalue = fbit_extract (buffer, *aoffsetp, 32);
	      flip_bytes (&Fvalue, 4);
	      *aoffsetp += 32;
	      sprintf (result, "$%g", Fvalue);
	      break;
	    case 'L':
	      Lvalue = dbit_extract (buffer, *aoffsetp, 64);
	      flip_bytes (&Lvalue, 8);
	      *aoffsetp += 64;
	      sprintf (result, "$%g", Lvalue);
	      break;
	    }
	  break;
	case 0x15:
	  disp1 = get_displacement (buffer, aoffsetp);
	  sprintf (result, "@|%d|", disp1);
	  break;
	case 0x16:
	  disp1 = get_displacement (buffer, aoffsetp);
	  disp2 = get_displacement (buffer, aoffsetp);
	  sprintf (result, "EXT(%d) + %d", disp1, disp2);
	  break;
	case 0x17:
	  sprintf (result, "tos");
	  break;
	case 0x18:
	  disp1 = get_displacement (buffer, aoffsetp);
	  sprintf (result, "%d(fp)", disp1);
	  break;
	case 0x19:
	  disp1 = get_displacement (buffer, aoffsetp);
	  sprintf (result, "%d(sp)", disp1);
	  break;
	case 0x1a:
	  disp1 = get_displacement (buffer, aoffsetp);
	  sprintf (result, "%d(sb)", disp1);
	  break;
	case 0x1b:
	  disp1 = get_displacement (buffer, aoffsetp);
	  sprintf (result, "|%d|", addr + disp1);
	  break;
	case 0x1c:
	case 0x1d:
	case 0x1e:
	case 0x1f:
	  index = bit_extract (buffer, *aoffsetp, 8);
	  *aoffsetp += 8;
	  print_insn_arg (d, *aoffsetp, aoffsetp, buffer, addr,
			  result);
	  {
	    static char	*ind[] = {"b", "w", "d", "q"};
	    char *off;

	    off = result + strlen (result);
	    sprintf (off, "[r%d:%s]", index & 7,
		     ind[addr_mode & 3]);
	  }
	  break;
	}
      break;
    case 'q':
      Ivalue = bit_extract (buffer, ioffset-4, 4);
      Ivalue = sign_extend (Ivalue, 4);
      sprintf (result, "%d", Ivalue);
      ioffset -= 4;
      break;
    case 'r':
      Ivalue = bit_extract (buffer, ioffset-3, 3);
      sprintf (result, "r%d", Ivalue&7);
      ioffset -= 3;
      break;
    case 'd':
      sprintf (result, "%d", get_displacement (buffer, aoffsetp));
      break;
    case 'p':
      sprintf (result, "%c%d%c", NEXT_IS_ADDR,
	       addr + get_displacement (buffer, aoffsetp),
	       NEXT_IS_ADDR);
      break;
    case 'i':
      Ivalue = bit_extract (buffer, *aoffsetp, 8);
      *aoffsetp += 8;
      sprintf (result, "0x%x", Ivalue);
      break;
    }
  return ioffset;
}

get_displacement (buffer, aoffsetp)
     char *buffer;
     int *aoffsetp;
{
  int Ivalue;

  Ivalue = bit_extract (buffer, *aoffsetp, 8);
  switch (Ivalue & 0xc0)
    {
    case 0x00:
    case 0x40:
      Ivalue = sign_extend (Ivalue, 7);
      *aoffsetp += 8;
      break;
    case 0x80:
      Ivalue = bit_extract (buffer, *aoffsetp, 16);
      flip_bytes (&Ivalue, 2);
      Ivalue = sign_extend (Ivalue, 14);
      *aoffsetp += 16;
      break;
    case 0xc0:
      Ivalue = bit_extract (buffer, *aoffsetp, 32);
      flip_bytes (&Ivalue, 4);
      Ivalue = sign_extend (Ivalue, 30);
      *aoffsetp += 32;
      break;
    }
  return Ivalue;
}

/* Return the number of locals in the current frame given a pc
   pointing to the enter instruction.  This is used in the macro
   FRAME_FIND_SAVED_REGS.  */

ns32k_localcount (enter_pc)
     CORE_ADDR enter_pc;
{
  unsigned char localtype;
  int localcount;

  localtype = read_memory_integer (enter_pc+2, 1);
  if ((localtype & 0x80) == 0)
    localcount = localtype;
  else if ((localtype & 0xc0) == 0x80)
    localcount = (((localtype & 0x3f) << 8)
		  | (read_memory_integer (enter_pc+3, 1) & 0xff));
  else
    localcount = (((localtype & 0x3f) << 24)
		  | ((read_memory_integer (enter_pc+3, 1) & 0xff) << 16)
		  | ((read_memory_integer (enter_pc+4, 1) & 0xff) << 8 )
		  | (read_memory_integer (enter_pc+5, 1) & 0xff));
  return localcount;
}

/*
 * Get the address of the enter opcode for the function
 * containing PC, if there is an enter for the function,
 * and if the pc is between the enter and exit.
 * Returns positive address if pc is between enter/exit,
 * 1 if pc before enter or after exit, 0 otherwise.
 */

CORE_ADDR
ns32k_get_enter_addr (pc)
     CORE_ADDR pc;
{
  CORE_ADDR enter_addr;
  unsigned char op;

  if (ABOUT_TO_RETURN (pc))
    return 1;		/* after exit */

  enter_addr = get_pc_function_start (pc);

  if (pc == enter_addr) 
    return 1;		/* before enter */

  op = read_memory_integer (enter_addr, 1);

  if (op != 0x82)
    return 0;		/* function has no enter/exit */

  return enter_addr;	/* pc is between enter and exit */
}
