/**
 * $Id$
 * Copyright (C) 2008 - 2009 Nils Asmussen
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

#include <common.h>
#include <mm.h>
#include <multiboot.h>
#include <proc.h>
#include <timer.h>
#include <cpu.h>
#include <vfs.h>
#include <vfsnode.h>
#include <vfsinfo.h>
#include <util.h>
#include <kheap.h>
#include <assert.h>
#include <string.h>

/* VFS-directory-entry (equal to the direntry of ext2) */
typedef struct {
	tVFSNodeNo nodeNo;
	u16 recLen;
	u16 nameLen;
	/* name follows (up to 255 bytes) */
} __attribute__((packed)) sVFSDirEntry;

/**
 * The read-callback for the proc-read-handler
 */
static void vfsinfo_procReadCallback(sVFSNode *node,u32 *dataSize,void **buffer);

/**
 * The read-callback for the thread-read-handler
 */
static void vfsinfo_threadReadCallback(sVFSNode *node,u32 *dataSize,void **buffer);

/**
 * The cpu-read-handler
 */
static s32 vfsinfo_cpuReadHandler(tTid tid,sVFSNode *node,u8 *buffer,u32 offset,u32 count);

/**
 * The read-callback for the cpu-read-handler
 */
static void vfsinfo_cpuReadCallback(sVFSNode *node,u32 *dataSize,void **buffer);

/**
 * The stats-read-handler
 */
static s32 vfsinfo_statsReadHandler(tTid tid,sVFSNode *node,u8 *buffer,u32 offset,u32 count);

/**
 * The read-callback for the stats-read-handler
 */
static void vfsinfo_statsReadCallback(sVFSNode *node,u32 *dataSize,void **buffer);

/**
 * The read-handler for the mem-usage-node
 */
static s32 vfsinfo_memUsageReadHandler(tTid tid,sVFSNode *node,u8 *buffer,u32 offset,u32 count);

/**
 * The read-callback for the VFS memusage-read-handler
 */
static void vfsinfo_memUsageReadCallback(sVFSNode *node,u32 *dataSize,void **buffer);

void vfsinfo_init(void) {
	tVFSNodeNo nodeNo;
	sVFSNode *sysNode;
	vfsn_resolvePath("system:/",&nodeNo,false);
	sysNode = vfsn_getNode(nodeNo);

	vfsn_createInfo(KERNEL_TID,sysNode,(char*)"memusage",vfsinfo_memUsageReadHandler);
	vfsn_createInfo(KERNEL_TID,sysNode,(char*)"cpu",vfsinfo_cpuReadHandler);
	vfsn_createInfo(KERNEL_TID,sysNode,(char*)"stats",vfsinfo_statsReadHandler);
}

s32 vfsinfo_procReadHandler(tTid tid,sVFSNode *node,u8 *buffer,u32 offset,u32 count) {
	/* don't use the cache here to prevent that one process occupies it for all others */
	/* (if the process doesn't call close() the cache will not be invalidated and therefore
	 * other processes might miss changes) */
	return vfs_readHelper(tid,node,buffer,offset,count,
			17 * 6 + 5 * 10 + MAX_PROC_NAME_LEN + 1,
			vfsinfo_procReadCallback);
}

static void vfsinfo_procReadCallback(sVFSNode *node,u32 *dataSize,void **buffer) {
	UNUSED(dataSize);
	sProc *p = proc_getByPid(atoi(node->parent->name));
	sStringBuffer buf;
	buf.dynamic = false;
	buf.str = *(char**)buffer;
	buf.size = 17 * 6 + 5 * 10 + MAX_PROC_NAME_LEN + 1;
	buf.len = 0;

	util_sprintf(
		&buf,
		"%-16s%u\n"
		"%-16s%u\n"
		"%-16s%s\n"
		"%-16s%u\n"
		"%-16s%u\n"
		"%-16s%u\n"
		,
		"Pid:",p->pid,
		"ParentPid:",p->parentPid,
		"Command:",p->command,
		"TextPages:",p->textPages,
		"DataPages:",p->dataPages,
		"StackPages:",p->stackPages
	);
}

s32 vfsinfo_threadReadHandler(tTid tid,sVFSNode *node,u8 *buffer,u32 offset,u32 count) {
	return vfs_readHelper(tid,node,buffer,offset,count,
			17 * 6 + 4 * 10 + 2 * 16 + 1,vfsinfo_threadReadCallback);
}

static void vfsinfo_threadReadCallback(sVFSNode *node,u32 *dataSize,void **buffer) {
	UNUSED(dataSize);
	sThread *t = thread_getById(atoi(node->name));
	u32 *uptr,*kptr;
	sStringBuffer buf;
	buf.dynamic = false;
	buf.str = *(char**)buffer;
	buf.size = 17 * 6 + 4 * 10 + 2 * 16 + 1;
	buf.len = 0;

	uptr = (u32*)&t->ucycleCount;
	kptr = (u32*)&t->kcycleCount;
	util_sprintf(
		&buf,
		"%-16s%u\n"
		"%-16s%u\n"
		"%-16s%u\n"
		"%-16s%u\n"
		"%-16s%08x%08x\n"
		"%-16s%08x%08x\n"
		,
		"Tid:",t->tid,
		"Pid:",t->proc->pid,
		"State:",t->state,
		"StackPages:",t->ustackPages,
		"UCPUCycles:",*(uptr + 1),*uptr,
		"KCPUCycles:",*(kptr + 1),*kptr
	);
}

static s32 vfsinfo_cpuReadHandler(tTid tid,sVFSNode *node,u8 *buffer,u32 offset,u32 count) {
	return vfs_readHelper(tid,node,buffer,offset,count,0,vfsinfo_cpuReadCallback);
}

static void vfsinfo_cpuReadCallback(sVFSNode *node,u32 *dataSize,void **buffer) {
	UNUSED(node);
	sStringBuffer buf;
	buf.dynamic = true;
	buf.str = NULL;
	buf.size = 0;
	buf.len = 0;
	cpu_sprintf(&buf);
	*buffer = buf.str;
	*dataSize = buf.len + 1;
}

static s32 vfsinfo_statsReadHandler(tTid tid,sVFSNode *node,u8 *buffer,u32 offset,u32 count) {
	return vfs_readHelper(tid,node,buffer,offset,count,17 * 5 + 4 * 10 + 1 + 1 * 16 + 1,
			vfsinfo_statsReadCallback);
}

static void vfsinfo_statsReadCallback(sVFSNode *node,u32 *dataSize,void **buffer) {
	UNUSED(dataSize);
	UNUSED(node);
	u32 *ptr;
	sStringBuffer buf;
	buf.dynamic = false;
	buf.str = *(char**)buffer;
	buf.size = 17 * 5 + 4 * 10 + 1 + 1 * 16 + 1;
	buf.len = 0;

	u64 cycles = cpu_rdtsc();
	ptr = (u32*)&cycles;
	util_sprintf(
		&buf,
		"%-16s%u\n"
		"%-16s%u\n"
		"%-16s%u\n"
		"%-16s%08x%08x\n"
		"%-16s%us\n"
		,
		"Processes:",proc_getCount(),
		"Threads:",thread_getCount(),
		"Interrupts:",intrpt_getCount(),
		"CPUCycles:",*(ptr + 1),*ptr,
		"UpTime:",timer_getIntrptCount() / TIMER_FREQUENCY
	);
}

static s32 vfsinfo_memUsageReadHandler(tTid tid,sVFSNode *node,u8 *buffer,u32 offset,u32 count) {
	return vfs_readHelper(tid,node,buffer,offset,count,(11 + 10 + 1) * 4 + 1,
			vfsinfo_memUsageReadCallback);
}

static void vfsinfo_memUsageReadCallback(sVFSNode *node,u32 *dataSize,void **buffer) {
	UNUSED(node);
	UNUSED(dataSize);
	sStringBuffer buf;
	buf.dynamic = false;
	buf.str = *(char**)buffer;
	buf.size = (11 + 10 + 1) * 4 + 1;
	buf.len = 0;

	u32 free = mm_getFreeFrmCount(MM_DEF | MM_DMA) << PAGE_SIZE_SHIFT;
	u32 total = mboot_getUsableMemCount();
	util_sprintf(
		&buf,
		"%-11s%10u\n"
		"%-11s%10u\n"
		"%-11s%10u\n"
		"%-11s%10u\n"
		,
		"Total:",total,
		"Used:",total - free,
		"Free:",free,
		"KHeapSize:",kheap_getUsedMem()
	);
}

s32 vfsinfo_dirReadHandler(tTid tid,sVFSNode *node,u8 *buffer,u32 offset,u32 count) {
	s32 byteCount;

	UNUSED(tid);
	vassert(node != NULL,"node == NULL");
	vassert(buffer != NULL,"buffer == NULL");

	/* not cached yet? */
	if(node->data.def.cache == NULL) {
		/* we need the number of bytes first */
		byteCount = 0;
		sVFSNode *n = NODE_FIRST_CHILD(node);
		while(n != NULL) {
			byteCount += sizeof(sVFSDirEntry) + strlen(n->name);
			n = n->next;
		}

		vassert((u32)byteCount < (u32)0xFFFF,"Overflow of size and pos detected");

		node->data.def.size = byteCount;
		node->data.def.pos = byteCount;
		if(byteCount > 0) {
			/* now allocate mem on the heap and copy all data into it */
			u8 *childs = (u8*)kheap_alloc(byteCount);
			if(childs == NULL) {
				node->data.def.size = 0;
				node->data.def.pos = 0;
			}
			else {
				u16 len;
				sVFSDirEntry *dirEntry = (sVFSDirEntry*)childs;
				node->data.def.cache = childs;
				n = NODE_FIRST_CHILD(node);
				while(n != NULL) {
					len = strlen(n->name);
					dirEntry->nodeNo = NADDR_TO_VNNO(n);
					dirEntry->nameLen = len;
					dirEntry->recLen = sizeof(sVFSDirEntry) + len;
					memcpy(dirEntry + 1,n->name,len);
					dirEntry = (sVFSDirEntry*)((u8*)dirEntry + dirEntry->recLen);
					n = n->next;
				}
			}
		}
	}

	if(offset > node->data.def.size)
		offset = node->data.def.size;
	byteCount = MIN(node->data.def.size - offset,count);
	if(byteCount > 0) {
		/* simply copy the data to the buffer */
		memcpy(buffer,(u8*)node->data.def.cache + offset,byteCount);
	}

	return byteCount;
}
