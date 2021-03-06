/**
 * $Id$
 * Copyright (C) 2008 - 2014 Nils Asmussen
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <mem/cache.h>
#include <mem/pagedir.h>
#include <mem/useraccess.h>
#include <mem/virtmem.h>
#include <task/elf.h>
#include <task/groups.h>
#include <task/proc.h>
#include <task/signals.h>
#include <task/thread.h>
#include <task/timer.h>
#include <task/uenv.h>
#include <vfs/node.h>
#include <vfs/vfs.h>
#include <common.h>
#include <errno.h>
#include <string.h>
#include <syscalls.h>
#include <util.h>

int Syscalls::getpid(Thread *t,IntrptStackFrame *stack) {
	SYSC_SUCCESS(stack,t->getProc()->getPid());
}

int Syscalls::getppid(A_UNUSED Thread *t,IntrptStackFrame *stack) {
	SYSC_SUCCESS(stack,t->getProc()->getParentPid());
}

int Syscalls::getuid(Thread *t,IntrptStackFrame *stack) {
	SYSC_SUCCESS(stack,t->getProc()->getUid());
}

int Syscalls::setuid(Thread *t,IntrptStackFrame *stack) {
	uid_t uid = (uid_t)SYSC_ARG1(stack);
	Proc *p = t->getProc();
	if(EXPECT_FALSE(p->getUid() != ROOT_UID))
		SYSC_ERROR(stack,-EPERM);

	p->setUid(uid);
	VFS::chownProcess(p->getPid(),p->getUid(),p->getGid());
	SYSC_SUCCESS(stack,0);
}

int Syscalls::getgid(Thread *t,IntrptStackFrame *stack) {
	SYSC_SUCCESS(stack,t->getProc()->getGid());
}

int Syscalls::setgid(Thread *t,IntrptStackFrame *stack) {
	gid_t gid = (gid_t)SYSC_ARG1(stack);
	Proc *p = t->getProc();
	if(EXPECT_FALSE(p->getUid() != ROOT_UID))
		SYSC_ERROR(stack,-EPERM);

	p->setGid(gid);
	VFS::chownProcess(p->getPid(),p->getUid(),p->getGid());
	SYSC_SUCCESS(stack,0);
}

int Syscalls::getgroups(Thread *t,IntrptStackFrame *stack) {
	size_t size = (size_t)SYSC_ARG1(stack);
	gid_t *list = (gid_t*)SYSC_ARG2(stack);
	pid_t pid = t->getProc()->getPid();
	if(EXPECT_FALSE(!PageDir::isInUserSpace((uintptr_t)list,sizeof(gid_t) * size)))
		SYSC_ERROR(stack,-EFAULT);

	size = Groups::get(pid,list,size);
	SYSC_SUCCESS(stack,size);
}

int Syscalls::setgroups(Thread *t,IntrptStackFrame *stack) {
	size_t size = (size_t)SYSC_ARG1(stack);
	const gid_t *list = (const gid_t*)SYSC_ARG2(stack);
	Proc *p = t->getProc();

	if(EXPECT_FALSE(!PageDir::isInUserSpace((uintptr_t)list,sizeof(gid_t) * size)))
		SYSC_ERROR(stack,-EFAULT);

	// root can set arbitrary groups
	if(p->getUid() != ROOT_UID) {
		// non-root users can only downgrade their groups
		for(size_t i = 0; i < size; ++i) {
			gid_t gid;
			if(UserAccess::readVar(&gid,list + i) < 0)
				SYSC_ERROR(stack,-EINVAL);
			if(!Groups::contains(p->getPid(),gid))
				SYSC_ERROR(stack,-EPERM);
		}
	}

	if(EXPECT_FALSE(!Groups::set(p->getPid(),size,list)))
		SYSC_ERROR(stack,-ENOMEM);
	SYSC_SUCCESS(stack,0);
}

int Syscalls::fork(A_UNUSED Thread *t,IntrptStackFrame *stack) {
	int res = Proc::clone(0);
	SYSC_RESULT(stack,res);
}

int Syscalls::waitchild(A_UNUSED Thread *t,IntrptStackFrame *stack) {
	/* better work on a copy in kernel memory. it's not worth the trouble here... */
	Proc::ExitState kstate;
	Proc::ExitState *state = (Proc::ExitState*)SYSC_ARG1(stack);
	pid_t pid = (pid_t)SYSC_ARG2(stack);
	int options = (int)SYSC_ARG3(stack);
	if(EXPECT_FALSE(state != NULL && !PageDir::isInUserSpace((uintptr_t)state,sizeof(Proc::ExitState))))
		SYSC_ERROR(stack,-EFAULT);

	int res = Proc::waitChild(&kstate,pid,options);
	if(state)
		UserAccess::write(state,&kstate,sizeof(kstate));
	SYSC_RESULT(stack,res);
}

int Syscalls::exec(Thread *t,IntrptStackFrame *stack) {
	int fd = (int)SYSC_ARG1(stack);
	const char *const *args = (const char *const *)SYSC_ARG2(stack);
	const char *const *env = (const char *const *)SYSC_ARG3(stack);
	Proc *p = t->getProc();

	ScopedFile file(p,fd);
	if(!file)
		SYSC_ERROR(stack,-EBADF);
	if((file->getFlags() & (VFS_EXEC | VFS_READ)) != (VFS_EXEC | VFS_READ))
		SYSC_ERROR(stack,-EACCES);
	int res = Proc::exec(&*file,fd,args,env);
	SYSC_RESULT(stack,res);
}
