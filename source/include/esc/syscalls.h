/**
 * $Id$
 */

#ifndef SYSCALLS_H_
#define SYSCALLS_H_

/* the syscall-numbers */
#define SYSCALL_PID				0
#define SYSCALL_PPID			1
#define SYSCALL_DEBUGCHAR		2
#define SYSCALL_FORK			3
#define SYSCALL_EXIT			4
#define SYSCALL_OPEN			5
#define SYSCALL_CLOSE			6
#define SYSCALL_READ			7
#define SYSCALL_REG				8
#define SYSCALL_CHGSIZE			9
#define SYSCALL_MAPPHYS			10
#define SYSCALL_WRITE			11
#define SYSCALL_YIELD			12
#define SYSCALL_DUPFD			13
#define SYSCALL_REDIRFD			14
#define SYSCALL_WAIT			15
#define SYSCALL_SETSIGH			16
#define SYSCALL_ACKSIG			17
#define SYSCALL_SENDSIG			18
#define SYSCALL_EXEC			19
#define SYSCALL_FCNTL			20
#define SYSCALL_LOADMODS		21
#define SYSCALL_SLEEP			22
#define SYSCALL_SEEK			23
#define SYSCALL_STAT			24
#define SYSCALL_DEBUG			25
#define SYSCALL_CREATESHMEM		26
#define SYSCALL_JOINSHMEM		27
#define SYSCALL_LEAVESHMEM		28
#define SYSCALL_DESTROYSHMEM	29
#define SYSCALL_GETCLIENTPROC	30
#define SYSCALL_LOCK			31
#define SYSCALL_UNLOCK			32
#define SYSCALL_STARTTHREAD		33
#define SYSCALL_GETTID			34
#define SYSCALL_GETTHREADCNT	35
#define SYSCALL_SEND			36
#define SYSCALL_RECEIVE			37
#define SYSCALL_GETCYCLES		38
#define SYSCALL_SYNC			39
#define SYSCALL_LINK			40
#define SYSCALL_UNLINK			41
#define SYSCALL_MKDIR			42
#define SYSCALL_RMDIR			43
#define SYSCALL_MOUNT			44
#define SYSCALL_UNMOUNT			45
#define SYSCALL_WAITCHILD		46
#define SYSCALL_TELL			47
#define SYSCALL_PIPE			48
#define SYSCALL_GETCONF			49
#define SYSCALL_GETWORK			50
#define SYSCALL_ISTERM			51
#define SYSCALL_JOIN			52
#define SYSCALL_SUSPEND			53
#define SYSCALL_RESUME			54
#define SYSCALL_FSTAT			55
#define SYSCALL_ADDREGION		56
#define SYSCALL_SETREGPROT		57
#define SYSCALL_NOTIFY			58
#define SYSCALL_GETCLIENTID		59
#define SYSCALL_WAITUNLOCK		60
#define SYSCALL_GETENVITO		61
#define SYSCALL_GETENVTO		62
#define SYSCALL_SETENV			63
#define SYSCALL_REQIOPORTS		64
#define SYSCALL_RELIOPORTS		65
#define SYSCALL_VM86INT			66

#endif /* SYSCALLS_H_ */