/* Perform arithmetic and other operations on values, for GDB.
   Copyright (C) 1986 Free Software Foundation, Inc.

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
#include "symtab.h"
#include "value.h"
#include "expression.h"


value value_x_binop ();

value
value_add (arg1, arg2)
	value arg1, arg2;
{
  register value val, valint, valptr;
  register int len;

  COERCE_ARRAY (arg1);
  COERCE_ARRAY (arg2);

  if ((TYPE_CODE (VALUE_TYPE (arg1)) == TYPE_CODE_PTR
       || TYPE_CODE (VALUE_TYPE (arg2)) == TYPE_CODE_PTR)
      &&
      (TYPE_CODE (VALUE_TYPE (arg1)) == TYPE_CODE_INT
       || TYPE_CODE (VALUE_TYPE (arg2)) == TYPE_CODE_INT))
    /* Exactly one argument is a pointer, and one is an integer.  */
    {
      if (TYPE_CODE (VALUE_TYPE (arg1)) == TYPE_CODE_PTR)
	{
	  valptr = arg1;
	  valint = arg2;
	}
      else
	{
	  valptr = arg2;
	  valint = arg1;
	}
      len = TYPE_LENGTH (TYPE_TARGET_TYPE (VALUE_TYPE (valptr)));
      if (len == 0) len = 1;	/* For (void *) */
      val = value_from_long (builtin_type_long,
			     value_as_long (valptr)
			     + (len * value_as_long (valint)));
      VALUE_TYPE (val) = VALUE_TYPE (valptr);
      return val;
    }

  return value_binop (arg1, arg2, BINOP_ADD);
}

value
value_sub (arg1, arg2)
	value arg1, arg2;
{
  register value val;

  COERCE_ARRAY (arg1);
  COERCE_ARRAY (arg2);

  if (TYPE_CODE (VALUE_TYPE (arg1)) == TYPE_CODE_PTR
      && 
      TYPE_CODE (VALUE_TYPE (arg2)) == TYPE_CODE_INT)
    {
      val = value_from_long (builtin_type_long,
			     value_as_long (arg1)
			     - TYPE_LENGTH (TYPE_TARGET_TYPE (VALUE_TYPE (arg1))) * value_as_long (arg2));
      VALUE_TYPE (val) = VALUE_TYPE (arg1);
      return val;
    }

  if (TYPE_CODE (VALUE_TYPE (arg1)) == TYPE_CODE_PTR
      && 
      VALUE_TYPE (arg1) == VALUE_TYPE (arg2))
    {
      val = value_from_long (builtin_type_long,
			     (value_as_long (arg1) - value_as_long (arg2))
			     / TYPE_LENGTH (TYPE_TARGET_TYPE (VALUE_TYPE (arg1))));
      return val;
    }

  return value_binop (arg1, arg2, BINOP_SUB);
}

/* Return the value of ARRAY[IDX].  */

value
value_subscript (array, idx)
     value array, idx;
{
  return value_ind (value_add (array, idx));
}

/* Check to see if either argument is a structure.  This is called so
   we know whether to go ahead with the normal binop or look for a 
   user defined function instead.

   For now, we do not overload the `=' operator.  */

int
binop_user_defined_p (op, arg1, arg2)
     enum exp_opcode op;
     value arg1, arg2;
{
  if (op == BINOP_ASSIGN)
    return 0;
  return (TYPE_CODE (VALUE_TYPE (arg1)) == TYPE_CODE_STRUCT
	  || TYPE_CODE (VALUE_TYPE (arg2)) == TYPE_CODE_STRUCT
	  || (TYPE_CODE (VALUE_TYPE (arg1)) == TYPE_CODE_REF
	      && TYPE_CODE (TYPE_TARGET_TYPE (VALUE_TYPE (arg1))) == TYPE_CODE_STRUCT)
	  || (TYPE_CODE (VALUE_TYPE (arg2)) == TYPE_CODE_REF
	      && TYPE_CODE (TYPE_TARGET_TYPE (VALUE_TYPE (arg2))) == TYPE_CODE_STRUCT));
}

/* Check to see if argument is a structure.  This is called so
   we know whether to go ahead with the normal unop or look for a 
   user defined function instead.

   For now, we do not overload the `&' operator.  */

int unop_user_defined_p (op, arg1)
     enum exp_opcode op;
     value arg1;
{
  if (op == UNOP_ADDR)
    return 0;
  return (TYPE_CODE (VALUE_TYPE (arg1)) == TYPE_CODE_STRUCT
	  || (TYPE_CODE (VALUE_TYPE (arg1)) == TYPE_CODE_REF
	      && TYPE_CODE (TYPE_TARGET_TYPE (VALUE_TYPE (arg1))) == TYPE_CODE_STRUCT));
}

/* We know either arg1 or arg2 is a structure, so try to find the right
   user defined function.  Create an argument vector that calls 
   arg1.operator @ (arg1,arg2) and return that value (where '@' is any
   binary operator which is legal for GNU C++).  */

value
value_x_binop (arg1, arg2, op, otherop)
     value arg1, arg2;
     int op, otherop;
{
  value * argvec;
  char *ptr;
  char tstr[13];
  
  COERCE_ENUM (arg1);
  COERCE_ENUM (arg2);

  /* now we know that what we have to do is construct our
     arg vector and find the right function to call it with.  */

  if (TYPE_CODE (VALUE_TYPE (arg1)) != TYPE_CODE_STRUCT)
    error ("friend functions not implemented yet");

  argvec = (value *) alloca (sizeof (value) * 4);
  argvec[1] = value_addr (arg1);
  argvec[2] = arg2;
  argvec[3] = 0;

  /* make the right function name up */  
  strcpy(tstr, "operator __");
  ptr = tstr+9;
  switch (op)
    {
    case BINOP_ADD:	strcpy(ptr,"+"); break;
    case BINOP_SUB:	strcpy(ptr,"-"); break;
    case BINOP_MUL:	strcpy(ptr,"*"); break;
    case BINOP_DIV:	strcpy(ptr,"/"); break;
    case BINOP_REM:	strcpy(ptr,"%"); break;
    case BINOP_LSH:	strcpy(ptr,"<<"); break;
    case BINOP_RSH:	strcpy(ptr,">>"); break;
    case BINOP_LOGAND:	strcpy(ptr,"&"); break;
    case BINOP_LOGIOR:	strcpy(ptr,"|"); break;
    case BINOP_LOGXOR:	strcpy(ptr,"^"); break;
    case BINOP_AND:	strcpy(ptr,"&&"); break;
    case BINOP_OR:	strcpy(ptr,"||"); break;
    case BINOP_MIN:	strcpy(ptr,"<?"); break;
    case BINOP_MAX:	strcpy(ptr,">?"); break;
    case BINOP_ASSIGN:	strcpy(ptr,"="); break;
    case BINOP_ASSIGN_MODIFY:	
      switch (otherop)
	{
	case BINOP_ADD:      strcpy(ptr,"+="); break;
	case BINOP_SUB:      strcpy(ptr,"-="); break;
	case BINOP_MUL:      strcpy(ptr,"*="); break;
	case BINOP_DIV:      strcpy(ptr,"/="); break;
	case BINOP_REM:      strcpy(ptr,"%="); break;
	case BINOP_LOGAND:   strcpy(ptr,"&="); break;
	case BINOP_LOGIOR:   strcpy(ptr,"|="); break;
	case BINOP_LOGXOR:   strcpy(ptr,"^="); break;
	default:
	  error ("Invalid binary operation specified.");
	}
      break;
    case BINOP_SUBSCRIPT: strcpy(ptr,"[]"); break;
    case BINOP_EQUAL:	  strcpy(ptr,"=="); break;
    case BINOP_NOTEQUAL:  strcpy(ptr,"!="); break;
    case BINOP_LESS:      strcpy(ptr,"<"); break;
    case BINOP_GTR:       strcpy(ptr,">"); break;
    case BINOP_GEQ:       strcpy(ptr,">="); break;
    case BINOP_LEQ:       strcpy(ptr,"<="); break;
    default:
      error ("Invalid binary operation specified.");
    }
  argvec[0] = value_struct_elt (arg1, argvec+1, tstr, "structure");
  if (argvec[0])
    return call_function (argvec[0], 2, argvec + 1);
  else error ("member function %s not found", tstr);
}

/* We know that arg1 is a structure, so try to find a unary user
   defined operator that matches the operator in question.  
   Create an argument vector that calls arg1.operator @ (arg1)
   and return that value (where '@' is (almost) any unary operator which
   is legal for GNU C++).  */

value
value_x_unop (arg1, op)
     value arg1;
     int op;
{
  value * argvec;
  char *ptr;
  char tstr[13];
  
  COERCE_ENUM (arg1);

  /* now we know that what we have to do is construct our
     arg vector and find the right function to call it with.  */

  if (TYPE_CODE (VALUE_TYPE (arg1)) != TYPE_CODE_STRUCT)
    error ("friend functions not implemented yet");

  argvec = (value *) alloca (sizeof (value) * 3);
  argvec[1] = value_addr (arg1);
  argvec[2] = 0;

  /* make the right function name up */  
  strcpy(tstr,"operator __");
  ptr = tstr+9;
  switch (op)
    {
    case UNOP_PREINCREMENT:	strcpy(ptr,"++"); break;
    case UNOP_PREDECREMENT:	strcpy(ptr,"++"); break;
    case UNOP_POSTINCREMENT:	strcpy(ptr,"++"); break;
    case UNOP_POSTDECREMENT:	strcpy(ptr,"++"); break;
    case UNOP_ZEROP:	strcpy(ptr,"!"); break;
    case UNOP_LOGNOT:	strcpy(ptr,"~"); break;
    case UNOP_NEG:	strcpy(ptr,"-"); break;
    default:
      error ("Invalid binary operation specified.");
    }
  argvec[0] = value_struct_elt (arg1, argvec+1, tstr, "structure");
  if (argvec[0])
    return call_function (argvec[0], 1, argvec + 1);
  else error ("member function %s not found", tstr);
}

/* Perform a binary operation on two integers or two floats.
   Does not support addition and subtraction on pointers;
   use value_add or value_sub if you want to handle those possibilities.  */

value
value_binop (arg1, arg2, op)
     value arg1, arg2;
     int op;
{
  register value val;

  COERCE_ENUM (arg1);
  COERCE_ENUM (arg2);

  if ((TYPE_CODE (VALUE_TYPE (arg1)) != TYPE_CODE_FLT
       &&
       TYPE_CODE (VALUE_TYPE (arg1)) != TYPE_CODE_INT)
      ||
      (TYPE_CODE (VALUE_TYPE (arg2)) != TYPE_CODE_FLT
       &&
       TYPE_CODE (VALUE_TYPE (arg2)) != TYPE_CODE_INT))
    error ("Argument to arithmetic operation not a number.");

  if (TYPE_CODE (VALUE_TYPE (arg1)) == TYPE_CODE_FLT
      ||
      TYPE_CODE (VALUE_TYPE (arg2)) == TYPE_CODE_FLT)
    {
      double v1, v2, v;
      v1 = value_as_double (arg1);
      v2 = value_as_double (arg2);
      switch (op)
	{
	case BINOP_ADD:
	  v = v1 + v2;
	  break;

	case BINOP_SUB:
	  v = v1 - v2;
	  break;

	case BINOP_MUL:
	  v = v1 * v2;
	  break;

	case BINOP_DIV:
	  v = v1 / v2;
	  break;

	default:
	  error ("Integer-only operation on floating point number.");
	}

      val = allocate_value (builtin_type_double);
      *(double *) VALUE_CONTENTS (val) = v;
    }
  else
    {
      LONGEST v1, v2, v;
      v1 = value_as_long (arg1);
      v2 = value_as_long (arg2);

      switch (op)
	{
	case BINOP_ADD:
	  v = v1 + v2;
	  break;

	case BINOP_SUB:
	  v = v1 - v2;
	  break;

	case BINOP_MUL:
	  v = v1 * v2;
	  break;

	case BINOP_DIV:
	  v = v1 / v2;
	  break;

	case BINOP_REM:
	  v = v1 % v2;
	  break;

	case BINOP_LSH:
	  v = v1 << v2;
	  break;

	case BINOP_RSH:
	  v = v1 >> v2;
	  break;

	case BINOP_LOGAND:
	  v = v1 & v2;
	  break;

	case BINOP_LOGIOR:
	  v = v1 | v2;
	  break;

	case BINOP_LOGXOR:
	  v = v1 ^ v2;
	  break;

	case BINOP_AND:
	  v = v1 && v2;
	  break;

	case BINOP_OR:
	  v = v1 || v2;
	  break;

	case BINOP_MIN:
	  v = v1 < v2 ? v1 : v2;
	  break;

	case BINOP_MAX:
	  v = v1 > v2 ? v1 : v2;
	  break;

	default:
	  error ("Invalid binary operation on numbers.");
	}

      val = allocate_value (BUILTIN_TYPE_LONGEST);
      *(LONGEST *) VALUE_CONTENTS (val) = v;
    }

  return val;
}

/* Simulate the C operator ! -- return 1 if ARG1 contains zeros.  */

int
value_zerop (arg1)
     value arg1;
{
  register int len;
  register char *p;

  COERCE_ARRAY (arg1);

  len = TYPE_LENGTH (VALUE_TYPE (arg1));
  p = VALUE_CONTENTS (arg1);

  while (--len >= 0)
    {
      if (*p++)
	break;
    }

  return len < 0;
}

/* Simulate the C operator == by returning a 1
   iff ARG1 and ARG2 have equal contents.  */

int
value_equal (arg1, arg2)
     register value arg1, arg2;

{
  register int len;
  register char *p1, *p2;
  enum type_code code1;
  enum type_code code2;

  COERCE_ARRAY (arg1);
  COERCE_ARRAY (arg2);

  code1 = TYPE_CODE (VALUE_TYPE (arg1));
  code2 = TYPE_CODE (VALUE_TYPE (arg2));

  if (code1 == TYPE_CODE_INT && code2 == TYPE_CODE_INT)
    return value_as_long (arg1) == value_as_long (arg2);
  else if ((code1 == TYPE_CODE_FLT || code1 == TYPE_CODE_INT)
	   && (code2 == TYPE_CODE_FLT || code2 == TYPE_CODE_INT))
    return value_as_double (arg1) == value_as_double (arg2);
  else if ((code1 == TYPE_CODE_PTR && code2 == TYPE_CODE_INT)
	   || (code2 == TYPE_CODE_PTR && code1 == TYPE_CODE_INT))
    return value_as_long (arg1) == value_as_long (arg2);
  else if (code1 == code2
	   && ((len = TYPE_LENGTH (VALUE_TYPE (arg1)))
	       == TYPE_LENGTH (VALUE_TYPE (arg2))))
    {
      p1 = VALUE_CONTENTS (arg1);
      p2 = VALUE_CONTENTS (arg2);
      while (--len >= 0)
	{
	  if (*p1++ != *p2++)
	    break;
	}
      return len < 0;
    }
  else
    error ("Invalid type combination in equality test.");
}

/* Simulate the C operator < by returning 1
   iff ARG1's contents are less than ARG2's.  */

int
value_less (arg1, arg2)
     register value arg1, arg2;
{
  register enum type_code code1;
  register enum type_code code2;

  COERCE_ARRAY (arg1);
  COERCE_ARRAY (arg2);

  code1 = TYPE_CODE (VALUE_TYPE (arg1));
  code2 = TYPE_CODE (VALUE_TYPE (arg2));

  if (code1 == TYPE_CODE_INT && code2 == TYPE_CODE_INT)
    return value_as_long (arg1) < value_as_long (arg2);
  else if ((code1 == TYPE_CODE_FLT || code1 == TYPE_CODE_INT)
	   && (code2 == TYPE_CODE_FLT || code2 == TYPE_CODE_INT))
    return value_as_double (arg1) < value_as_double (arg2);
  else if ((code1 == TYPE_CODE_PTR || code1 == TYPE_CODE_INT)
	   && (code2 == TYPE_CODE_PTR || code2 == TYPE_CODE_INT))
    return value_as_long (arg1) < value_as_long (arg2);
  else
    error ("Invalid type combination in ordering comparison.");
}

/* The unary operators - and ~.  Both free the argument ARG1.  */

value
value_neg (arg1)
     register value arg1;
{
  register struct type *type;

  COERCE_ENUM (arg1);

  type = VALUE_TYPE (arg1);

  if (TYPE_CODE (type) == TYPE_CODE_FLT)
    return value_from_double (type, - value_as_double (arg1));
  else if (TYPE_CODE (type) == TYPE_CODE_INT)
    return value_from_long (type, - value_as_long (arg1));
  else
    error ("Argument to negate operation not a number.");
}

value
value_lognot (arg1)
     register value arg1;
{
  COERCE_ENUM (arg1);

  if (TYPE_CODE (VALUE_TYPE (arg1)) != TYPE_CODE_INT)
    error ("Argument to complement operation not an integer.");

  return value_from_long (VALUE_TYPE (arg1), ~ value_as_long (arg1));
}

