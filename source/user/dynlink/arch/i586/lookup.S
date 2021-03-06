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

#include <sys/asm.h>

.section .text

.extern lookup_resolve
.extern getpid_wrapper

BEGIN_FUNC(lookup_resolveStart)
#ifndef CALLTRACE_PID
	push	%eax
	push	%edx
	push	%ecx
	call	lookup_resolve
	push	%eax
	mov		4(%esp),%ecx
	mov		8(%esp),%edx
	mov		12(%esp),%eax
	add		$24,%esp								// remove args of lookup_resolve from stack
	jmp		*-24(%esp)								// jump to function
#else
	push	8(%esp)									// push ret-addr for the call
	push	%eax
	push	%edx
	push	%ecx
	call	lookup_resolve
	push	%eax
	call	getpid_wrapper							// get pid
	cmp		$CALLTRACE_PID,%eax						// if not the requested pid skip the whole stuff
	jne		1f
	mov		(lookupDepth),%ecx
	cmp		$100,%ecx								// we have only 100 places...
	jae		1f
	mov		$lookupStack,%edx
	lea		(%edx,%ecx,4),%edx						// load addr to store ret-addr at
	add		$1,%ecx									// inc depth
	mov		%ecx,(lookupDepth)						// store
	mov		28(%esp),%ecx							// get real return-addr
	mov		%ecx,(%edx)								// save
	mov		$lookup_resolveFinish,%ecx				// replace with our own
	mov		%ecx,28(%esp)
1:
	mov		4(%esp),%ecx
	mov		8(%esp),%edx
	mov		12(%esp),%eax
	add		$28,%esp								// remove args of lookup_resolve from stack
	jmp		*-28(%esp)								// jump to function
END_FUNC(lookup_resolveStart)

BEGIN_FUNC(lookup_resolveFinish)
	.extern lookup_tracePop
	push	%eax
	call	lookup_tracePop
	pop		%eax
	mov		(lookupDepth),%ecx						// load depth
	sub		$1,%ecx									// decrement
	mov		%ecx,(lookupDepth)						// store
	mov		$lookupStack,%edx						// get stack
	lea		(%edx,%ecx,4),%edx						// build addr to our ret-addr
	mov		(%edx),%ecx								// get ret-addr
	jmp		*%ecx									// jump to that addr
END_FUNC(lookup_resolveFinish)
#endif

#if defined(CALLTRACE_PID)
.section .data
lookupDepth:
	.long 0
lookupStack:
	.rept 100
		.long 0
	.endr
#endif
