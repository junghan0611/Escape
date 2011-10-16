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

#include <sys/common.h>
#include <sys/task/proc.h>
#include <sys/mem/pmem.h>
#include <sys/mem/paging.h>
#include <sys/mem/kheap.h>
#include <sys/mem/vmm.h>
#include <sys/intrpt.h>
#include <sys/ksymbols.h>
#include <sys/video.h>
#include <sys/klock.h>
#include <sys/util.h>
#include <sys/log.h>
#include <stdarg.h>
#include <string.h>

/* source: http://en.wikipedia.org/wiki/Linear_congruential_generator */
static uint randa = 1103515245;
static uint randc = 12345;
static uint lastRand = 0;
static klock_t randLock;

int util_rand(void) {
	int res;
	klock_aquire(&randLock);
	lastRand = randa * lastRand + randc;
	res = (int)((uint)(lastRand / 65536) % 32768);
	klock_release(&randLock);
	return res;
}

void util_srand(uint seed) {
	klock_aquire(&randLock);
	lastRand = seed;
	klock_release(&randLock);
}

void util_printStackTraceShort(const sFuncCall *trace) {
	if(trace) {
		size_t i;
		for(i = 0; trace->addr != 0 && i < 5; i++) {
			sSymbol *sym = ksym_getSymbolAt(trace->addr);
			if(sym->address)
				log_printf("%s",sym->funcName);
			else
				log_printf("%Px",trace->addr);
			trace++;
			if(trace->addr)
				log_printf(" ");
		}
	}
}

void util_printStackTrace(const sFuncCall *trace) {
	if(trace) {
		if(trace->addr < KERNEL_AREA)
			vid_printf("User-Stacktrace:\n");
		else
			vid_printf("Kernel-Stacktrace:\n");

		while(trace->addr != 0) {
			vid_printf("\t%p -> %p (%s)\n",(trace + 1)->addr,trace->funcAddr,trace->funcName);
			trace++;
		}
	}
}

void util_dumpMem(const void *addr,size_t dwordCount) {
	ulong *ptr = (ulong*)addr;
	while(dwordCount-- > 0) {
		vid_printf("%p: 0x%0*lx\n",ptr,sizeof(ulong) * 2,*ptr);
		ptr++;
	}
}

void util_dumpBytes(const void *addr,size_t byteCount) {
	size_t i = 0;
	uchar *ptr = (uchar*)addr;
	for(i = 0; byteCount-- > 0; i++) {
		vid_printf("%02x ",*ptr);
		ptr++;
		if(i % 16 == 15)
			vid_printf("\n");
	}
}
