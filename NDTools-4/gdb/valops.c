/* Perform non-arithmetic operations on values, for GDB.
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
#include "stdio.h"
#include "defs.h"
#include "param.h"
#include "symtab.h"
#include "value.h"
#include "frame.h"
#include "inferior.h"

/* Cast value ARG2 to type TYPE and return as a value.
   More general than a C cast: accepts any two types of the same length,
   and if ARG2 is an lvalue it can be cast into anything at all.  */

value
value_cast (type, arg2)
     struct type *type;
     register value arg2;
{
  register enum type_code code1;
  register enum type_code code2;
  register int scalar;

  /* Coerce arrays but not enums.  Enums will work as-is
     and coercing them would cause an infinite recursion.  */
  if (TYPE_CODE (VALUE_TYPE (arg2)) != TYPE_CODE_ENUM)
    COERCE_ARRAY (arg2);

  code1 = TYPE_CODE (type);
  code2 = TYPE_CODE (VALUE_TYPE (arg2));
  scalar = (code2 == TYPE_CODE_INT || code2 == TYPE_CODE_FLT
	    || code2 == TYPE_CODE_ENUM);

  if (code1 == TYPE_CODE_FLT && scalar)
    return value_from_double (type, value_as_double (arg2));
  else if ((code1 == TYPE_CODE_INT || code1 == TYPE_CODE_ENUM)
	   && (scalar || code2 == TYPE_CODE_PTR))
    return value_from_long (type, value_as_long (arg2));
  else if (TYPE_LENGTH (type) == TYPE_LENGTH (VALUE_TYPE (arg2)))
    {
      VALUE_TYPE (arg2) = type;
      return arg2;
    }
  else if (VALUE_LVAL (arg2) == lval_memory)
    {
      return value_at (type, VALUE_ADDRESS (arg2) + VALUE_OFFSET (arg2));
    }
  else
    error ("Invalid cast.");
}

/* Return the value with a specified type located at specified address.  */

value
value_at (type, addr)
     struct type *type;
     CORE_ADDR addr;
{
  register value val = allocate_value (type);
  int temp;

  temp = read_memory (addr, VALUE_CONTENTS (val), TYPE_LENGTH (type));
  if (temp)
    {
      if (have_inferior_p ())
	print_sys_errmsg ("ptrace", temp);
      /* Actually, address between addr and addr + len was out of bounds. */
      error ("Cannot read memory: address 0x%x out of bounds.", addr);
    }

  VALUE_LVAL (val) = lval_memory;
  VALUE_ADDRESS (val) = addr;

  return val;
}

/* Store the contents of FROMVAL into the location of TOVAL.
   Return a new value with the location of TOVAL and contents of FROMVAL.  */

value
value_assign (toval, fromval)
     register value toval, fromval;
{
  register struct type *type = VALUE_TYPE (toval);
  register value val;
  char raw_buffer[MAX_REGISTER_RAW_SIZE];
  char virtual_buffer[MAX_REGISTER_VIRTUAL_SIZE];
  int use_buffer = 0;

  COERCE_ARRAY (fromval);

  if (VALUE_LVAL (toval) != lval_internalvar)
    fromval = value_cast (type, fromval);

  /* If TOVAL is a special machine register requiring conversion
     of program values to a special raw format,
     convert FROMVAL's contents now, with result in `raw_buffer',
     and set USE_BUFFER to the number of bytes to write.  */

  if (VALUE_REGNO (toval) >= 0
      && REGISTER_CONVERTIBLE (VALUE_REGNO (toval)))
    {
      int regno = VALUE_REGNO (toval);
      if (VALUE_TYPE (fromval) != REGISTER_VIRTUAL_TYPE (regno))
	fromval = value_cast (REGISTER_VIRTUAL_TYPE (regno), fromval);
      bcopy (VALUE_CONTENTS (fromval), virtual_buffer,
	     REGISTER_VIRTUAL_SIZE (regno));
      REGISTER_CONVERT_TO_RAW (regno, virtual_buffer, raw_buffer);
      use_buffer = REGISTER_RAW_SIZE (regno);
    }

  switch (VALUE_LVAL (toval))
    {
    case lval_internalvar:
      set_internalvar (VALUE_INTERNALVAR (toval), fromval);
      break;

    case lval_internalvar_component:
      set_internalvar_component (VALUE_INTERNALVAR (toval),
				 VALUE_OFFSET (toval),
				 VALUE_BITPOS (toval),
				 VALUE_BITSIZE (toval),
				 fromval);
      break;

    case lval_memory:
      if (VALUE_BITSIZE (toval))
	{
	  int val;
	  read_memory (VALUE_ADDRESS (toval) + VALUE_OFFSET (toval),
		       &val, sizeof val);
	  modify_field (&val, (int) value_as_long (fromval),
			VALUE_BITPOS (toval), VALUE_BITSIZE (toval));
	  write_memory (VALUE_ADDRESS (toval) + VALUE_OFFSET (toval),
			&val, sizeof val);
	}
      else if (use_buffer)
	write_memory (VALUE_ADDRESS (toval) + VALUE_OFFSET (toval),
		      raw_buffer, use_buffer);
      else
	write_memory (VALUE_ADDRESS (toval) + VALUE_OFFSET (toval),
		      VALUE_CONTENTS (fromval), TYPE_LENGTH (type));
      break;

    case lval_register:
      if (VALUE_BITSIZE (toval))
	{
	  int val;

	  read_register_bytes (VALUE_ADDRESS (toval) + VALUE_OFFSET (toval),
			       &val, sizeof val);
	  modify_field (&val, (int) value_as_long (fromval),
			VALUE_BITPOS (toval), VALUE_BITSIZE (toval));
	  write_register_bytes (VALUE_ADDRESS (toval) + VALUE_OFFSET (toval),
				&val, sizeof val);
	}
      else if (use_buffer)
	write_register_bytes (VALUE_ADDRESS (toval) + VALUE_OFFSET (toval),
			      raw_buffer, use_buffer);
      else
	write_register_bytes (VALUE_ADDRESS (toval) + VALUE_OFFSET (toval),
			      VALUE_CONTENTS (fromval), TYPE_LENGTH (type));
      break;

    default:
      error ("Left side of = operation is not an lvalue.");
    }

  /* Return a value just like TOVAL except with the contents of FROMVAL.  */

  val = allocate_value (type);
  bcopy (toval, val, VALUE_CONTENTS (val) - (char *) val);
  bcopy (VALUE_CONTENTS (fromval), VALUE_CONTENTS (val), TYPE_LENGTH (type));

  return val;
}

/* Extend a value VAL to COUNT repetitions of its type.  */

value
value_repeat (arg1, count)
     value arg1;
     int count;
{
  register value val;

  if (VALUE_LVAL (arg1) != lval_memory)
    error ("Only values in memory can be extended with '@'.");
  if (count < 1)
    error ("Invalid number %d of repetitions.", count);

  val = allocate_repeat_value (VALUE_TYPE (arg1), count);

  read_memory (VALUE_ADDRESS (arg1) + VALUE_OFFSET (arg1),
	       VALUE_CONTENTS (val),
	       TYPE_LENGTH (VALUE_TYPE (val)) * count);
  VALUE_LVAL (val) = lval_memory;
  VALUE_ADDRESS (val) = VALUE_ADDRESS (arg1) + VALUE_OFFSET (arg1);

  return val;
}

value
value_of_variable (var)
     struct symbol *var;
{
  return read_var_value (var, (CORE_ADDR) 0);
}

/* Given a value which is an array, return a value which is
   a pointer to its first element.  */

value
value_coerce_array (arg1)
     value arg1;
{
  register struct type *type;
  register value val;

  if (VALUE_LVAL (arg1) != lval_memory)
    error ("Attempt to take address of value not located in memory.");

  /* Get type of elements.  */
  if (TYPE_CODE (VALUE_TYPE (arg1)) == TYPE_CODE_ARRAY)
    type = TYPE_TARGET_TYPE (VALUE_TYPE (arg1));
  else
    /* A phony array made by value_repeat.
       Its type is the type of the elements, not an array type.  */
    type = VALUE_TYPE (arg1);

  /* Get the type of the result.  */
  type = lookup_pointer_type (type);
  val = value_from_long (builtin_type_long,
		       (LONGEST) (VALUE_ADDRESS (arg1) + VALUE_OFFSET (arg1)));
  VALUE_TYPE (val) = type;
  return val;
}

/* Return a pointer value for the object for which ARG1 is the contents.  */

value
value_addr (arg1)
     value arg1;
{
  register struct type *type;
  register value val, arg1_coerced;

  /* Taking the address of an array is really a no-op
     once the array is coerced to a pointer to its first element.  */
  arg1_coerced = arg1;
  COERCE_ARRAY (arg1_coerced);
  if (arg1 != arg1_coerced)
    return arg1_coerced;

  if (VALUE_LVAL (arg1) != lval_memory)
    error ("Attempt to take address of value not located in memory.");

  /* Get the type of the result.  */
  type = lookup_pointer_type (VALUE_TYPE (arg1));
  val = value_from_long (builtin_type_long,
		(LONGEST) (VALUE_ADDRESS (arg1) + VALUE_OFFSET (arg1)));
  VALUE_TYPE (val) = type;
  return val;
}

/* Given a value of a pointer type, apply the C unary * operator to it.  */

value
value_ind (arg1)
     value arg1;
{
  /* Must do this before COERCE_ARRAY, otherwise an infinite loop
     will result */
  if (TYPE_CODE (VALUE_TYPE (arg1)) == TYPE_CODE_REF)
    return value_at (TYPE_TARGET_TYPE (VALUE_TYPE (arg1)),
		     (CORE_ADDR) value_as_long (arg1));

  COERCE_ARRAY (arg1);

  if (TYPE_CODE (VALUE_TYPE (arg1)) == TYPE_CODE_MEMBER)
    error ("not implemented: member types in value_ind");

  /* Allow * on an integer so we can cast it to whatever we want.  */
  if (TYPE_CODE (VALUE_TYPE (arg1)) == TYPE_CODE_INT)
    return value_at (builtin_type_long,
		     (CORE_ADDR) value_as_long (arg1));
  else if (TYPE_CODE (VALUE_TYPE (arg1)) == TYPE_CODE_PTR)
    return value_at (TYPE_TARGET_TYPE (VALUE_TYPE (arg1)),
		     (CORE_ADDR) value_as_long (arg1));
  else if (TYPE_CODE (VALUE_TYPE (arg1)) == TYPE_CODE_REF)
    return value_at (TYPE_TARGET_TYPE (VALUE_TYPE (arg1)),
		     (CORE_ADDR) value_as_long (arg1));
  error ("Attempt to take contents of a non-pointer value.");
}

/* Pushing small parts of stack frames.  */

/* Push one word (the size of object that a register holds).  */

CORE_ADDR
push_word (sp, buffer)
     CORE_ADDR sp;
     REGISTER_TYPE buffer;
{
  register int len = sizeof (REGISTER_TYPE);

#if 1 INNER_THAN 2
  sp -= len;
  write_memory (sp, &buffer, len);
#else /* stack grows upward */
  write_memory (sp, &buffer, len);
  sp += len;
#endif /* stack grows upward */

  return sp;
}

/* Push LEN bytes with data at BUFFER.  */

CORE_ADDR
push_bytes (sp, buffer, len)
     CORE_ADDR sp;
     char *buffer;
     int len;
{
#if 1 INNER_THAN 2
  sp -= len;
  write_memory (sp, buffer, len);
#else /* stack grows upward */
  write_memory (sp, buffer, len);
  sp += len;
#endif /* stack grows upward */

  return sp;
}

/* Push onto the stack the specified value VALUE.  */

CORE_ADDR
value_push (sp, arg)
     register CORE_ADDR sp;
     value arg;
{
  register int len = TYPE_LENGTH (VALUE_TYPE (arg));

#if 1 INNER_THAN 2
  sp -= len;
  write_memory (sp, VALUE_CONTENTS (arg), len);
#else /* stack grows upward */
  write_memory (sp, VALUE_CONTENTS (arg), len);
  sp += len;
#endif /* stack grows upward */

  return sp;
}

/* Perform the standard coercions that are specified
   for arguments to be passed to C functions.  */

value
value_arg_coerce (arg)
     value arg;
{
  register struct type *type;

  COERCE_ENUM (arg);

  type = VALUE_TYPE (arg);

  if (TYPE_CODE (type) == TYPE_CODE_INT
      && TYPE_LENGTH (type) < sizeof (int))
    return value_cast (builtin_type_int, arg);

  if (type == builtin_type_float)
    return value_cast (builtin_type_double, arg);

  return arg;
}

/* Push the value ARG, first coercing it as an argument
   to a C function.  */

CORE_ADDR
value_arg_push (sp, arg)
     register CORE_ADDR sp;
     value arg;
{
  return value_push (sp, value_arg_coerce (arg));
}

/* Perform a function call in the inferior.
   ARGS is a vector of values of arguments (NARGS of them).
   FUNCTION is a value, the function to be called.
   Returns a value representing what the function returned.
   May fail to return, if a breakpoint or signal is hit
   during the execution of the function.  */

value
call_function (function, nargs, args)
     value function;
     int nargs;
     value *args;
{
  register CORE_ADDR sp;
  register int i;
  CORE_ADDR start_sp;
  static REGISTER_TYPE dummy[] = CALL_DUMMY;
  REGISTER_TYPE dummy1[sizeof dummy / sizeof (REGISTER_TYPE)];
  CORE_ADDR old_sp;
  struct type *value_type;
  unsigned char struct_return;
  CORE_ADDR struct_addr;
  struct inferior_status inf_status;
  struct cleanup *old_chain;

  save_inferior_status (&inf_status, 1);
  old_chain = make_cleanup (restore_inferior_status, &inf_status);

  PUSH_DUMMY_FRAME;

  old_sp = sp = read_register (SP_REGNUM);

#if 1 INNER_THAN 2		/* Stack grows down */
  sp -= sizeof dummy;
  start_sp = sp;
#else				/* Stack grows up */
  start_sp = sp;
  sp += sizeof dummy;
#endif

  {
    register CORE_ADDR funaddr;
    register struct type *ftype = VALUE_TYPE (function);
    register enum type_code code = TYPE_CODE (ftype);

    /* If it's a member function, just look at the function
       part of it.  */
    if (code == TYPE_CODE_MEMBER)
      {
	ftype = TYPE_TARGET_TYPE (ftype);
	code = TYPE_CODE (ftype);
      }

    /* Determine address to call.  */
    if (code == TYPE_CODE_FUNC)
      {
	funaddr = VALUE_ADDRESS (function);
	value_type = TYPE_TARGET_TYPE (ftype);
      }
    else if (code == TYPE_CODE_PTR)
      {
	funaddr = value_as_long (function);
	if (TYPE_CODE (TYPE_TARGET_TYPE (ftype))
	    == TYPE_CODE_FUNC)
	  value_type = TYPE_TARGET_TYPE (TYPE_TARGET_TYPE (ftype));
	else
	  value_type = builtin_type_int;
      }
    else if (code == TYPE_CODE_INT)
      {
	/* Handle the case of functions lacking debugging info.
	   Their values are characters since their addresses are char */
	if (TYPE_LENGTH (ftype) == 1)
	  funaddr = value_as_long (value_addr (function));
	else
	  /* Handle integer used as address of a function.  */
	  funaddr = value_as_long (function);

	value_type = builtin_type_int;
      }
    else
      error ("Invalid data type for function to be called.");

    /* Are we returning a value using a structure return or a normal
       value return? */

    struct_return = using_struct_return (function, funaddr, value_type);

    /* Create a call sequence customized for this function
       and the number of arguments for it.  */
    bcopy (dummy, dummy1, sizeof dummy);
#ifdef TEXT_DUMMY
    /* Put the code at the end of text if there is room in the last page */
    {
      extern CORE_ADDR text_end;
      if (0x1000 - (text_end & 0xFFF) >= sizeof(dummy)) 
	start_sp = text_end;
#ifndef CALL_DUMMY_VARIABLE
      else
	error ("text segment full -- no place to put call");
#endif
    }
#endif	/* TEXT_DUMMY */
#ifdef CALL_DUMMY_VARIABLE
    /* Need to save where we put call_dummy for future reference */
    {
      extern CORE_ADDR call_dummy_set;	 /* true if dummy call being done */
      extern CORE_ADDR call_dummy_start; /* address of call dummy code */

      call_dummy_set = 1;
      call_dummy_start = start_sp;
    }
#endif	/* CALL_DUMMY_VARIABLE */
    FIX_CALL_DUMMY (dummy1, start_sp, funaddr, nargs, value_type);
  }

  write_memory (start_sp, dummy1, sizeof dummy);

#ifdef convex
  /* Convex Unix prohibits executing in the stack segment. */
  /* Hope there is empty room at the top of the text segment. */
  {
    extern CORE_ADDR text_end;
    static checked = 0;
    if (!checked)
      for (start_sp = text_end - sizeof dummy; start_sp < text_end; ++start_sp)
	if (read_memory_integer (start_sp, 1) != 0)
	  error ("text segment full -- no place to put call");
    checked = 1;
    start_sp = text_end - sizeof dummy;
    write_memory (start_sp, dummy1, sizeof dummy);
  }
#else /* !convex */
#ifdef STACK_ALIGN
  /* If stack grows down, we must leave a hole at the top. */
  {
    int len = 0;

    /* Reserve space for the return structure to be written on the
       stack, if necessary */

    if (struct_return)
      len += TYPE_LENGTH (value_type);
    
#ifdef STACK_ARG_BYTES
    STACK_ARG_BYTES(i, args, nargs, struct_return);
    len += i;
#else
    for (i = nargs - 1; i >= 0; i--)
      len += TYPE_LENGTH (VALUE_TYPE (args[i]));
#endif
#ifdef CALL_DUMMY_STACK_ADJUST
    len += CALL_DUMMY_STACK_ADJUST;
#endif
#if 1 INNER_THAN 2
    sp -= STACK_ALIGN (len) - len;
#else
    sp += STACK_ALIGN (len) - len;
#endif
  }
#endif /* STACK_ALIGN */

    /* Reserve space for the return structure to be written on the
       stack, if necessary */

    if (struct_return)
      {
#if 1 INNER_THAN 2
	sp -= TYPE_LENGTH (value_type);
	struct_addr = sp;
#else
	struct_addr = sp;
	sp += TYPE_LENGTH (value_type);
#endif
      }
    
#ifdef PASS_FUNCTION_ARGUMENTS
  PASS_FUNCTION_ARGUMENTS(args, nargs, struct_return);
#else
  for (i = nargs - 1; i >= 0; i--)
    sp = value_arg_push (sp, args[i]);
#endif

#ifdef CALL_DUMMY_STACK_ADJUST
#if 1 INNER_THAN 2
  sp -= CALL_DUMMY_STACK_ADJUST;
#else
  sp += CALL_DUMMY_STACK_ADJUST;
#endif
#endif /* CALL_DUMMY_STACK_ADJUST */
#endif /* !convex */

  /* Store the address at which the structure is supposed to be
     written.  Note that this (and the code which reserved the space
     above) assumes that gcc was used to compile this function.  Since
     it doesn't cost us anything but space and if the function is pcc
     it will ignore this value, we will make that assumption.

     Also note that on some machines (like the sparc) pcc uses this
     convention in a slightly twisted way also.  */

  if (struct_return)
    STORE_STRUCT_RETURN (struct_addr, sp);

  /* Write the stack pointer.  This is here because the statement above
     might fool with it */
  write_register (SP_REGNUM, sp);

  /* Figure out the value returned by the function.  */
  {
    char retbuf[REGISTER_BYTES];

    /* Execute the stack dummy routine, calling FUNCTION.
       When it is done, discard the empty frame
       after storing the contents of all regs into retbuf.  */
    run_stack_dummy (start_sp + CALL_DUMMY_START_OFFSET, retbuf);

    do_cleanups (old_chain);

    return value_being_returned (value_type, retbuf, struct_return);
  }
}

/* Create a value for a string constant:
   Call the function malloc in the inferior to get space for it,
   then copy the data into that space
   and then return the address with type char *.
   PTR points to the string constant data; LEN is number of characters.  */

value
value_string (ptr, len)
     char *ptr;
     int len;
{
  register value val;
  register struct symbol *sym;
  value blocklen;
  register char *copy = (char *) alloca (len + 1);
  char *i = ptr;
  register char *o = copy, *ibeg = ptr;
  register int c;

  /* Copy the string into COPY, processing escapes.
     We could not conveniently process them in expread
     because the string there wants to be a substring of the input.  */

  while (i - ibeg < len)
    {
      c = *i++;
      if (c == '\\')
	{
	  c = parse_escape (&i);
	  if (c == -1)
	    continue;
	}
      *o++ = c;
    }
  *o = 0;

  /* Get the length of the string after escapes are processed.  */

  len = o - copy;

  /* Find the address of malloc in the inferior.  */

  sym = lookup_symbol ("malloc", 0, VAR_NAMESPACE, 0);
  if (sym != 0)
    {
      if (SYMBOL_CLASS (sym) != LOC_BLOCK)
	error ("\"malloc\" exists in this program but is not a function.");
      val = value_of_variable (sym);
    }
  else
    {
      register int i;
      for (i = 0; i < misc_function_count; i++)
	if (!strcmp (misc_function_vector[i].name, "malloc"))
	  break;
      if (i < misc_function_count)
	val = value_from_long (builtin_type_long,
			     (LONGEST) misc_function_vector[i].address);
      else
	error ("String constants require the program to have a function \"malloc\".");
    }

  blocklen = value_from_long (builtin_type_int, (LONGEST) (len + 1));
  val = call_function (val, 1, &blocklen);
  if (value_zerop (val))
    error ("No memory available for string constant.");
  write_memory ((CORE_ADDR) value_as_long (val), copy, len + 1);
  VALUE_TYPE (val) = lookup_pointer_type (builtin_type_char);
  return val;
}

/* Given ARG1, a value of type (pointer to a)* structure/union,
   extract the component named NAME from the ultimate target structure/union
   and return it as a value with its appropriate type.
   ERR is used in the error message if ARG1's type is wrong.

   C++: ARGS is a list of argument types to aid in the selection of
   an appropriate method. Also, handle derived types.

   ERR is an error message to be printed in case the field is not found.  */

value
value_struct_elt (arg1, args, name, err)
     register value arg1, *args;
     char *name;
     char *err;
{
  register struct type *t;
  register int i;
  int found = 0;

  struct type *baseclass;

  COERCE_ARRAY (arg1);

  t = VALUE_TYPE (arg1);

  /* Follow pointers until we get to a non-pointer.  */

  while (TYPE_CODE (t) == TYPE_CODE_PTR || TYPE_CODE (t) == TYPE_CODE_REF)
    {
      arg1 = value_ind (arg1);
      COERCE_ARRAY (arg1);
      t = VALUE_TYPE (arg1);
    }

  if (TYPE_CODE (t) == TYPE_CODE_MEMBER)
    error ("not implemented: member type in value_struct_elt");

  if (TYPE_CODE (t) != TYPE_CODE_STRUCT
      && TYPE_CODE (t) != TYPE_CODE_UNION)
    error ("Attempt to extract a component of a value that is not a %s.", err);

  baseclass = t;

  if (!args)
    {
      /*  if there are no arguments ...do this...  */

      /*  Try as a variable first, because if we succeed, there
	  is less work to be done.  */
      while (t)
	{
	  for (i = TYPE_NFIELDS (t) - 1; i >= 0; i--)
	    {
	      if (!strcmp (TYPE_FIELD_NAME (t, i), name))
		{
		  found = 1;
		  break;
		}
	    }

	  if (i >= 0)
	    return TYPE_FIELD_STATIC (t, i)
	      ? value_static_field (t, name, i) : value_field (arg1, i);

	  if (TYPE_N_BASECLASSES (t) == 0)
	    break;

	  t = TYPE_BASECLASS (t, 1);
	  VALUE_TYPE (arg1) = t; /* side effect! */
	}

      /* C++: If it was not found as a data field, then try to
         return it as a pointer to a method.  */
      t = baseclass;
      VALUE_TYPE (arg1) = t;	/* side effect! */

      if (destructor_name_p (name, t))
	error ("use `info method' command to print out value of destructor");

      while (t)
	{
	  for (i = TYPE_NFN_FIELDS (t) - 1; i >= 0; --i)
	    {
	      if (! strcmp (TYPE_FN_FIELDLIST_NAME (t, i), name))
		{
		  error ("use `info method' command to print value of method \"%s\"", name);
		}
	    }

	  if (TYPE_N_BASECLASSES (t) == 0)
	    break;

	  t = TYPE_BASECLASS (t, 1);
	}

      if (found == 0)
	error("there is no field named %s", name);
      return 0;
    }

  if (destructor_name_p (name, t))
    {
      if (!args[1])
	{
	  /* destructors are a special case.  */
	  return (value)value_fn_field (arg1, 0, TYPE_FN_FIELDLIST_LENGTH (t, 0));
	}
      else
	{
	  error ("destructor should not have any argument");
	}
    }

  /*   This following loop is for methods with arguments.  */
  while (t)
    {
      /* Look up as method first, because that is where we
	 expect to find it first.  */
      for (i = TYPE_NFN_FIELDS (t) - 1; i >= 0; i--)
	{
	  struct fn_field *f = TYPE_FN_FIELDLIST1 (t, i);

	  if (!strcmp (TYPE_FN_FIELDLIST_NAME (t, i), name))
	    {
	      int j;
	      struct fn_field *f = TYPE_FN_FIELDLIST1 (t, i);

	      found = 1;
	      for (j = TYPE_FN_FIELDLIST_LENGTH (t, i) - 1; j >= 0; --j)
		{
		  if (!typecmp (TYPE_FN_FIELD_ARGS (f, j), args))
		    {
		      if (TYPE_FN_FIELD_VIRTUAL_P (f, j))
			return (value)value_virtual_fn_field (arg1, f, j, t);
		      else
			return (value)value_fn_field (arg1, i, j);
		    }
		}
	    }
	}

      if (TYPE_N_BASECLASSES (t) == 0)
	break;
      
      t = TYPE_BASECLASS (t, 1);
      VALUE_TYPE (arg1) = t;	/* side effect! */
    }

  if (found)
    {
      error ("Structure method %s not defined for arglist.", name);
      return 0;
    }
  else
    {
      /* See if user tried to invoke data as function */
      t = baseclass;
      while (t)
	{
	  for (i = TYPE_NFIELDS (t) - 1; i >= 0; i--)
	    {
	      if (!strcmp (TYPE_FIELD_NAME (t, i), name))
		{
		  found = 1;
		  break;
		}
	    }

	  if (i >= 0)
	    return TYPE_FIELD_STATIC (t, i)
	      ? value_static_field (t, name, i) : value_field (arg1, i);

	  if (TYPE_N_BASECLASSES (t) == 0)
	    break;

	  t = TYPE_BASECLASS (t, 1);
	  VALUE_TYPE (arg1) = t; /* side effect! */
	}
      error ("Structure has no component named %s.", name);
    }
}

/* C++: return 1 is NAME is a legitimate name for the destructor
   of type TYPE.  If TYPE does not have a destructor, or
   if NAME is inappropriate for TYPE, an error is signaled.  */
int
destructor_name_p (name, type)
     char *name;
     struct type *type;
{
  /* destructors are a special case.  */
  char *dname = TYPE_NAME (type);

  if (name[0] == '~')
    {
      if (! TYPE_HAS_DESTRUCTOR (type))
	error ("type `%s' does not have destructor defined",
	       TYPE_NAME (type));
      /* Skip past the "struct " at the front.  */
      while (*dname++ != ' ') ;
      if (strcmp (dname, name+1))
	error ("destructor specification error");
      else
	return 1;
    }
  return 0;
}

/* C++: Given ARG1, a value of type (pointer to a)* structure/union,
   return 1 if the component named NAME from the ultimate
   target structure/union is defined, otherwise, return 0.  */

int
check_field (arg1, name)
     register value arg1;
     char *name;
{
  register struct type *t;
  register int i;
  int found = 0;

  struct type *baseclass;

  COERCE_ARRAY (arg1);

  t = VALUE_TYPE (arg1);

  /* Follow pointers until we get to a non-pointer.  */

  while (TYPE_CODE (t) == TYPE_CODE_PTR || TYPE_CODE (t) == TYPE_CODE_REF)
    {
      arg1 = value_ind (arg1);
      COERCE_ARRAY (arg1);
      t = VALUE_TYPE (arg1);
    }

  if (TYPE_CODE (t) == TYPE_CODE_MEMBER)
    error ("not implemented: member type in check_field");

  if (TYPE_CODE (t) != TYPE_CODE_STRUCT
      && TYPE_CODE (t) != TYPE_CODE_UNION)
    error ("Internal error: `this' is not an aggregate");

  baseclass = t;

  while (t)
    {
      for (i = TYPE_NFIELDS (t) - 1; i >= 0; i--)
	{
	  if (!strcmp (TYPE_FIELD_NAME (t, i), name))
	    {
	      return 1;
	    }
	}
      if (TYPE_N_BASECLASSES (t) == 0)
	break;

      t = TYPE_BASECLASS (t, 1);
      VALUE_TYPE (arg1) = t;	/* side effect! */
    }

  /* C++: If it was not found as a data field, then try to
     return it as a pointer to a method.  */
  t = baseclass;
  VALUE_TYPE (arg1) = t;	/* side effect! */

  /* Destructors are a special case.  */
  if (destructor_name_p (name, t))
    return 1;

  while (t)
    {
      for (i = TYPE_NFN_FIELDS (t) - 1; i >= 0; --i)
	{
	  if (!strcmp (TYPE_FN_FIELDLIST_NAME (t, i), name))
	    return 1;
	}

      if (TYPE_N_BASECLASSES (t) == 0)
	break;

      t = TYPE_BASECLASS (t, 1);
    }
  return 0;
}

/* C++: Given an aggregate type DOMAIN, and a member name NAME,
   return the address of this member as a pointer to member
   type.  If INTYPE is non-null, then it will be the type
   of the member we are looking for.  This will help us resolve
   pointers to member functions.  */

value
value_struct_elt_for_address (domain, intype, name)
     struct type *domain, *intype;
     char *name;
{
  register struct type *t = domain;
  register int i;
  int found = 0;
  value v;

  struct type *baseclass;

  if (TYPE_CODE (t) != TYPE_CODE_STRUCT
      && TYPE_CODE (t) != TYPE_CODE_UNION)
    error ("Internal error: non-aggregate type to value_struct_elt_for_address");

  baseclass = t;

  while (t)
    {
      for (i = TYPE_NFIELDS (t) - 1; i >= 0; i--)
	{
	  if (!strcmp (TYPE_FIELD_NAME (t, i), name))
	    {
	      if (TYPE_FIELD_PACKED (t, i))
		error ("pointers to bitfield members not allowed");

	      v = value_from_long (builtin_type_int,
				   (LONGEST) (TYPE_FIELD_BITPOS (t, i) >> 3));
	      VALUE_TYPE (v) = lookup_pointer_type (
		      lookup_member_type (TYPE_FIELD_TYPE (t, i), baseclass));
	      return v;
	    }
	}

      if (TYPE_N_BASECLASSES (t) == 0)
	break;

      t = TYPE_BASECLASS (t, 1);
    }

  /* C++: If it was not found as a data field, then try to
     return it as a pointer to a method.  */
  t = baseclass;

  /* Destructors are a special case.  */
  if (destructor_name_p (name, t))
    {
      error ("pointers to destructors not implemented yet");
    }

  /* Perform all necessary dereferencing.  */
  while (intype && TYPE_CODE (intype) == TYPE_CODE_PTR)
    intype = TYPE_TARGET_TYPE (intype);

  while (t)
    {
      for (i = TYPE_NFN_FIELDS (t) - 1; i >= 0; --i)
	{
	  if (!strcmp (TYPE_FN_FIELDLIST_NAME (t, i), name))
	    {
	      int j = TYPE_FN_FIELDLIST_LENGTH (t, i);
	      struct fn_field *f = TYPE_FN_FIELDLIST1 (t, i);

	      if (intype == 0 && j > 1)
		error ("non-unique member `%s' requires type instantiation", name);
	      if (intype)
		{
		  while (j--)
		    if (TYPE_FN_FIELD_TYPE (f, j) == intype)
		      break;
		  if (j < 0)
		    error ("no member function matches that type instantiation");
		}
	      else
		j = 0;

	      if (TYPE_FN_FIELD_VIRTUAL_P (f, j))
		{
		  v = value_from_long (builtin_type_long,
				       (LONGEST) TYPE_FN_FIELD_VOFFSET (f, j));
		}
	      else
		{
		  struct symbol *s = lookup_symbol (TYPE_FN_FIELD_PHYSNAME (f, j),
						    0, VAR_NAMESPACE, 0);
		  v = locate_var_value (s, 0);
		}
	      VALUE_TYPE (v) = lookup_pointer_type (lookup_member_type (TYPE_FN_FIELD_TYPE (f, j), baseclass));
	      return v;
	    }
	}

      if (TYPE_N_BASECLASSES (t) == 0)
	break;

      t = TYPE_BASECLASS (t, 1);
    }
  return 0;
}

/* Compare two argument lists and return the position in which they differ,
   or zero if equal. Note that we ignore the first argument, which is
   the type of the instance variable. This is because we want to handle
   derived classes. This is not entirely correct: we should actually
   check to make sure that a requested operation is type secure,
   shouldn't we? */
int typecmp(t1, t2)
     struct type *t1[];
     value t2[];
{
  int i;

  if (t1[0]->code == TYPE_CODE_VOID) return 0;
  if (!t1[1]) return 0;
  for (i = 1; t1[i] && t1[i]->code != TYPE_CODE_VOID; i++)
    {
      if (! t2[i]
	  || t1[i]->code != t2[i]->type->code
	  || t1[i]->target_type != t2[i]->type->target_type)
	{
	  return i+1;
	}
    }
  if (!t1[i]) return 0;
  return t2[i] ? i+1 : 0;
}

/* C++: return the value of the class instance variable, if one exists.
   Flag COMPLAIN signals an error if the request is made in an
   inappropriate context.  */
value
value_of_this (complain)
     int complain;
{
  extern FRAME selected_frame;
  struct symbol *func, *sym;
  char *funname = 0;
  struct block *b;
  int i;

  if (selected_frame == 0)
    if (complain)
      error ("no frame selected");
    else return 0;

  func = get_frame_function (selected_frame);
  if (func)
    funname = SYMBOL_NAME (func);
  else
    if (complain)
      error ("no `this' in nameless context");
    else return 0;

  b = SYMBOL_BLOCK_VALUE (func);
  i = BLOCK_NSYMS (b);
  if (i <= 0)
    if (complain)
      error ("no args, no `this'");
    else return 0;

  sym = BLOCK_SYM (b, 0);
  if (strncmp ("$this", SYMBOL_NAME (sym), 5))
    if (complain)
      error ("current stack frame not in method");
    else return 0;

  return read_var_value (sym, selected_frame);
}
