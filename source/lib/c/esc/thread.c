/**
 * $Id$
 * Copyright (C) 2008 - 2011 Nils Asmussen
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

#include <esc/common.h>
#include <esc/thread.h>
#include <esc/sllist.h>
#include <stdlib.h>

#define HASHMAP_SIZE	64

typedef struct {
	tid_t tid;
	uint key;
	void *val;
} sThreadVal;

/**
 * Assembler-routines
 */
extern int _lock(uint ident,bool global,uint flags);
extern int _unlock(uint ident,bool global);
extern int _waitunlock(sWaitObject *objects,size_t objCount,uint ident,bool global);
extern int _getcycles(uint64_t *res);

static sSLList *tvalmap[HASHMAP_SIZE];

bool setThreadVal(uint key,void *val) {
	tid_t tid = gettid();
	sSLList *list;
	sThreadVal *tval = (sThreadVal*)malloc(sizeof(sThreadVal));
	if(!tval)
		return false;

	list = tvalmap[tid % HASHMAP_SIZE];
	if(!list) {
		list = tvalmap[tid % HASHMAP_SIZE] = sll_create();
		if(!list) {
			free(tval);
			return false;
		}
	}

	tval->tid = tid;
	tval->key = key;
	tval->val = val;
	if(!sll_append(list,tval)) {
		free(tval);
		return false;
	}
	return true;
}

void *getThreadVal(uint key) {
	sSLNode *n;
	tid_t tid = gettid();
	sSLList *list = tvalmap[tid % HASHMAP_SIZE];
	if(list == NULL)
		return NULL;
	for(n = sll_begin(list); n != NULL; n = n->next) {
		sThreadVal *tval = (sThreadVal*)n->data;
		if(tval->tid == tid && tval->key == key)
			return tval->val;
	}
	return NULL;
}

int waitm(sWaitObject *objects,size_t objCount) {
	return waitmuntil(objects,objCount,0);
}

int wait(uint events,evobj_t object) {
	sWaitObject obj;
	obj.events = events;
	obj.object = object;
	return waitmuntil(&obj,1,0);
}

int waituntil(uint events,evobj_t object,time_t max) {
	sWaitObject obj;
	obj.events = events;
	obj.object = object;
	return waitmuntil(&obj,1,max);
}

int lock(uint ident,uint flags) {
	return syscall3(SYSCALL_LOCK,ident,false,flags);
}

int lockg(uint ident,uint flags) {
	return syscall3(SYSCALL_LOCK,ident,true,flags);
}

int waitunlock(uint events,evobj_t object,uint ident) {
	sWaitObject obj;
	obj.events = events;
	obj.object = object;
	return syscall4(SYSCALL_WAITUNLOCK,(ulong)&obj,1,ident,false);
}

int waitunlockg(uint events,evobj_t object,uint ident) {
	sWaitObject obj;
	obj.events = events;
	obj.object = object;
	return syscall4(SYSCALL_WAITUNLOCK,(ulong)&obj,1,ident,true);
}

int unlock(uint ident) {
	return syscall2(SYSCALL_UNLOCK,ident,false);
}

int unlockg(uint ident) {
	return syscall2(SYSCALL_UNLOCK,ident,true);
}

uint64_t getcycles(void) {
	uint64_t res;
	syscall1(SYSCALL_GETCYCLES,(ulong)&res);
	return res;
}
