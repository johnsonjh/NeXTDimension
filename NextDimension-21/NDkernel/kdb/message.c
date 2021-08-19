/*
 ****************************************************************
 * Mach Operating System
 * Copyright (c) 1986 Carnegie-Mellon University
 *
 * This software was developed by the Mach operating system
 * project at Carnegie-Mellon University's Department of Computer
 * Science. Software contributors as of May 1986 include Mike Accetta,
 * Robert Baron, William Bolosky, Jonathan Chew, David Golub,
 * Glenn Marcy, Richard Rashid, Avie Tevanian and Michael Young.
 *
 * Some software in these files are derived from sources other
 * than CMU.  Previous copyright and other source notices are
 * preserved below and permission to use such software is
 * dependent on licenses from those institutions.
 *
 * Permission to use the CMU portion of this software for
 * any non-commercial research and development purpose is
 * granted with the understanding that appropriate credit
 * will be given to CMU, the Mach project and its authors.
 * The Mach project would appreciate being notified of any
 * modifications and of redistribution of this software so that
 * bug fixes and enhancements may be distributed to users.
 *
 * All other rights are reserved to Carnegie-Mellon University.
 ****************************************************************
 */
/*
 *
 *	UNIX debugger
 *
 */

#include	"mac.h"
#include	"mode.h"

msg		VERSION =  "\nVERSION GAcK/i860	DATE 5/16/90\n";

msg		BADMOD	=  "bad modifier";
msg		BADCOM	=  "bad command";
msg		BADSYM	=  "symbol not found";
msg		BADLOC	=  "automatic variable not found";
msg		NOCFN	=  "c routine not found";
msg		NOMATCH	=  "cannot locate value";
msg		NOBKPT	=  "no breakpoint set";
msg		BADKET	=  "unexpected ')'";
msg		NOADR	=  "address expected";
msg		NOPCS	=  "no process";
msg		BADVAR	=  "bad variable";
msg		EXBKPT	=  "too many breakpoints";
msg		A68BAD	=  "bad a68 frame";
msg		A68LNK	=  "bad a68 link";
msg		ADWRAP	=  "address wrap around";
msg		BADEQ	=  "unexpected `='";
msg		BADWAIT	=  "wait error: process disappeared!";
msg		ENDPCS	=  "process terminated";
msg		NOFORK	=  "try again";
msg		BADSYN	=  "syntax error";
msg		NOEOR	=  "newline expected";
msg		SZBKPT	=  "bkpt: command too long";
msg		BADFIL	=  "bad file format";
msg		BADNAM	=  "not enough space for symbols";
msg		LONGFIL	=  "filename too long";
msg		NOTOPEN	=  "cannot open";
msg		BADMAG	=  "bad core magic number";
msg		TOODEEP =  "$<< nesting too deep";
