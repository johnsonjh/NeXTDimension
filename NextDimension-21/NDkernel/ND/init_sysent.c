/* 
 * Mach Operating System
 * Copyright (c) 1987 Carnegie-Mellon University
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 *
 */

/*
 * Copyright (c) 1982, 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

/*
 * System call switch table.
 */
#include "i860/proc.h"
#include "sys/systm.h"
#include "sys/errno.h"

	/* serial or parallel system call */
#define syss(fn,no) {no, 0, fn}
#define sysp(fn,no) {no, 1, fn}

int	nosys();
int	nullsys();
int	errsys();


struct sysent sysent[] = {
	syss(nosys,0),			/*   0 = indir */
	syss(nosys,0),			/*   1 = exit */
	syss(nosys,0),			/*   2 = fork */
	syss(nosys,0),			/*   3 = read */
	syss(nosys,0),			/*   4 = write */
	syss(nosys,0),			/*   5 = open */
	syss(nosys,0),			/*   6 = close */
	syss(nosys,0),			/*   7 = wait4 */
	syss(nosys,0),			/*   8 = creat */
	syss(nosys,0),			/*   9 = link */
	syss(nosys,0),			/*  10 = unlink */
	syss(nosys,0),			/*  11 = execv */
	syss(nosys,0),			/*  12 = chdir */
	syss(nosys,0),	/*  13 = old time */
	syss(nosys,0),			/*  14 = mknod */
	syss(nosys,0),			/*  15 = chmod */
	syss(nosys,0),			/*  16 = chown; now 3 args */
	syss(nosys,0),			/*  17 = old break */
		syss(nosys,0),	/*  18 = old stat */
	syss(nosys,0),			/*  19 = lseek */
	syss(nosys,0),			/*  20 = getpid */
	syss(nosys,0),			/*  21 = old mount */
	syss(nosys,0),		/*  22 = old umount */
		syss(nosys,0),	/*  23 = old setuid */
	syss(nosys,0),			/*  24 = getuid */
		syss(nosys,0),	/*  25 = old stime */
	syss(nosys,0),			/*  26 = ptrace */
		syss(nosys,0),	/*  27 = old alarm */
		syss(nosys,0),	/*  28 = old fstat */
		syss(nosys,0),	/*  29 = opause */
		syss(nosys,0),	/*  30 = old utime */
	syss(nosys,0),			/*  31 = was stty */
	syss(nosys,0),			/*  32 = was gtty */
	syss(nosys,0),			/*  33 = access */
		syss(nosys,0),	/*  34 = old nice */
		syss(nosys,0),	/*  35 = old ftime */
	syss(nosys,0),			/*  36 = sync */
	syss(nosys,0),			/*  37 = kill */
	syss(nosys,0),			/*  38 = stat */
		syss(nosys,0),	/*  39 = old setpgrp */
	syss(nosys,0),			/*  40 = lstat */
	syss(nosys,0),			/*  41 = dup */
	syss(nosys,0),			/*  42 = pipe */
		syss(nosys,0),	/*  43 = old times */
	syss(nosys,0),			/*  44 = profil */
	syss(nosys,0),			/*  45 = nosys */
		syss(nosys,0),	/*  46 = old setgid */
	syss(nosys,0),			/*  47 = getgid */
		syss(nosys,0),	/*  48 = old sig */
	syss(nosys,0),			/*  49 = reserved for USG */
	syss(nosys,0),			/*  50 = reserved for USG */
	syss(nosys,0),		/*  51 = turn acct off/on */
	syss(nosys,0),			/*  52 = old set phys addr */
	syss(nosys,0),			/*  53 = old lock in core */
	syss(nosys,0),			/*  54 = ioctl */
	syss(nosys,0),			/*  55 = reboot */
	syss(nosys,0),			/*  56 = old mpxchan */
	syss(nosys,0),		/*  57 = symlink */
	syss(nosys,0),		/*  58 = readlink */
	syss(nosys,0),			/*  59 = execve */
	syss(nosys,0),			/*  60 = umask */
	syss(nosys,0),			/*  61 = chroot */
	syss(nosys,0),			/*  62 = fstat */
	syss(nosys,0),			/*  63 = used internally */
	syss(nosys,0),		/*  64 = getpagesize */
	syss(nosys,0),			/*  65 = mremap */
	syss(nosys,0),			/*  66 = vfork */
	syss(nosys,0),			/*  67 = old vread */
	syss(nosys,0),			/*  68 = old vwrite */
	syss(nosys,0),			/*  69 = sbrk */
	syss(nosys,0),			/*  70 = sstk */
	syss(nosys,0),			/*  71 = mmap */
	syss(nosys,0),		/*  72 = old vadvise */
	syss(nosys,0),			/*  73 = munmap */
	syss(nosys,0),		/*  74 = mprotect */
	syss(nosys,0),		/*  75 = madvise */
	syss(nosys,0),		/*  76 = vhangup */
		syss(nosys,0),	/*  77 = old vlimit */
	syss(nosys,0),		/*  78 = mincore */
	syss(nosys,0),		/*  79 = getgroups */
	syss(nosys,0),		/*  80 = setgroups */
	syss(nosys,0),		/*  81 = getpgrp */
	syss(nosys,0),		/*  82 = setpgrp */
	syss(nosys,0),		/*  83 = setitimer */
	syss(nosys,0),			/*  84 = wait */
	syss(nosys,0),			/*  85 = swapon */
	syss(nosys,0),		/*  86 = getitimer */
	syss(nosys,0),		/*  87 = gethostname */
	syss(nosys,0),		/*  88 = sethostname */
	syss(nosys,0),		/*  89 = getdtablesize */
	syss(nosys,0),			/*  90 = dup2 */
	syss(nosys,0),		/*  91 = getdopt */
	syss(nosys,0),			/*  92 = fcntl */
	syss(nosys,0),			/*  93 = select */
	syss(nosys,0),		/*  94 = setdopt */
	syss(nosys,0),			/*  95 = fsync */
	syss(nosys,0),		/*  96 = setpriority */
	syss(nosys,0),			/*  97 = socket */
	syss(nosys,0),		/*  98 = connect */
	syss(nosys,0),			/*  99 = accept */
	syss(nosys,0),		/* 100 = getpriority */
	syss(nosys,0),			/* 101 = send */
	syss(nosys,0),			/* 102 = recv */
	syss(nosys,0),		/* 103 = sigreturn */
	syss(nosys,0),			/* 104 = bind */
	syss(nosys,0),		/* 105 = setsockopt */
	syss(nosys,0),			/* 106 = listen */
		syss(nosys,0),	/* 107 = old vtimes */
	syss(nosys,0),			/* 108 = sigvec */
	syss(nosys,0),		/* 109 = sigblock */
	syss(nosys,0),		/* 110 = sigsetmask */
	syss(nosys,0),		/* 111 = sigpause */
	syss(nosys,0),		/* 112 = sigstack */
	syss(nosys,0),		/* 113 = recvmsg */
	syss(nosys,0),		/* 114 = sendmsg */
	syss(nosys,0),			/* 115 = nosys */
	syss(nosys,0),		/* 116 = gettimeofday */
	syss(nosys,0),		/* 117 = getrusage */
	syss(nosys,0),		/* 118 = getsockopt */
	syss(nosys,0),			/* 119 = nosys */
	syss(nosys,0),			/* 120 = readv */
	syss(nosys,0),			/* 121 = writev */
	syss(nosys,0),		/* 122 = settimeofday */
	syss(nosys,0),			/* 123 = fchown */
	syss(nosys,0),			/* 124 = fchmod */
	syss(nosys,0),		/* 125 = recvfrom */
	syss(nosys,0),		/* 126 = setreuid */
	syss(nosys,0),		/* 127 = setregid */
	syss(nosys,0),			/* 128 = rename */
	syss(nosys,0),		/* 129 = truncate */
	syss(nosys,0),		/* 130 = ftruncate */
	syss(nosys,0),			/* 131 = flock */
	syss(nosys,0),			/* 132 = nosys */
	syss(nosys,0),			/* 133 = sendto */
	syss(nosys,0),		/* 134 = shutdown */
	syss(nosys,0),		/* 135 = socketpair */
	syss(nosys,0),			/* 136 = mkdir */
	syss(nosys,0),			/* 137 = rmdir */
	syss(nosys,0),			/* 138 = utimes */
	syss(nosys,0),		/* 139 = 4.2 signal cleanup */
	syss(nosys,0),		/* 140 = adjtime */
	syss(nosys,0),		/* 141 = getpeername */
	syss(nosys,0),		/* 142 = gethostid */
	syss(nosys,0),			/* 143 = old sethostid */
	syss(nosys,0),		/* 144 = getrlimit */
	syss(nosys,0),		/* 145 = setrlimit */
	syss(nosys,0),			/* 146 = killpg */
	syss(nosys,0),			/* 147 = nosys */
	syss(nosys,0),/* XXX */	/* 148 = old quota */
	syss(nosys,0),/* XXX */	/* 149 = old qquota */
	syss(nosys,0),		/* 150 = getsockname */
	/*
	 * Syscalls 151-180 inclusive are reserved for vendor-specific
	 * system calls.  (This includes various calls added for compatibity
	 * with other Unix variants.)
	 */
	syss(nosys,0),			/* 151 */
	syss(nosys,0),			/* 152 */
	syss(nosys,0),			/* 153 */
	syss(nosys,0),			/* 154 */
	syss(nosys,0),			/* 155 */
	syss(nosys,0),			/* 156 */
	syss(nosys,0),			/* 157 */
	syss(nosys,0),			/* 158 */
	syss(nosys,0),			/* 159 */
	syss(nosys,0),			/* 160 */
	syss(nosys,0),			/* 161 */
	syss(nosys,0),			/* 162 */
	syss(nosys,0),			/* 163 */
	syss(nosys,0),			/* 164 */
	syss(nosys,0),			/* 165 */
	syss(nosys,0),			/* 166 */
	syss(nosys,0),			/* 167 */
	syss(nosys,0),			/* 168 */
	syss(nosys,0),			/* 169 */
	syss(nosys,0),			/* 170 */
	syss(nosys,0),			/* 171 */
	syss(nosys,0),			/* 172 */
	syss(nosys,0),			/* 173 */
	syss(nosys,0),			/* 174 */
	syss(nosys,0),			/* 175 */
	syss(nosys,0),			/* 176 */
	syss(nosys,0),			/* 177 */
	syss(nosys,0),			/* 178 */
	syss(nosys,0),			/* 179 */
	syss(nosys,0),			/* 180 */
	syss(nosys,0),			/* 181 */
};
int	nsysent = sizeof (sysent) / sizeof (sysent[0]);

/*
 * Null system calls. Not invalid, just not configured.
 */
nosys()	/* For now, just say it's invalid. */
{
	extern struct proc *P;
	P->p_error = EINVAL;
}

errsys()
{
	extern struct proc *P;
	P->p_error = EINVAL;
}

nullsys()
{
}

