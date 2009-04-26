;
; $Id$
; Copyright (C) 2008 - 2009 Nils Asmussen
;
; This program is free software; you can redistribute it and/or
; modify it under the terms of the GNU General Public License
; as published by the Free Software Foundation; either version 2
; of the License, or (at your option) any later version.
;
; This program is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; GNU General Public License for more details.
;
; You should have received a copy of the GNU General Public License
; along with this program; if not, write to the Free Software
; Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
;

[BITS 32]

; the syscall-numbers
SYSCALL_PID						equ 0
SYSCALL_PPID					equ 1
SYSCALL_DEBUGCHAR			equ 2
SYSCALL_FORK					equ 3
SYSCALL_EXIT					equ	4
SYSCALL_OPEN					equ 5
SYSCALL_CLOSE					equ 6
SYSCALL_READ					equ 7
SYSCALL_REG						equ 8
SYSCALL_UNREG					equ 9
SYSCALL_CHGSIZE				equ 10
SYSCALL_MAPPHYS				equ	11
SYSCALL_WRITE					equ	12
SYSCALL_YIELD					equ 13
SYSCALL_GETCLIENT			equ 14
SYSCALL_REQIOPORTS		equ 15
SYSCALL_RELIOPORTS		equ 16
SYSCALL_DUPFD					equ	17
SYSCALL_REDIRFD				equ 18
SYSCALL_WAIT					equ 19
SYSCALL_SETSIGH				equ	20
SYSCALL_UNSETSIGH			equ	21
SYSCALL_ACKSIG				equ	22
SYSCALL_SENDSIG				equ 23
SYSCALL_EXEC					equ 24
SYSCALL_EOF						equ 25
SYSCALL_LOADMODS			equ	26
SYSCALL_SLEEP					equ 27
SYSCALL_CREATENODE		equ	28
SYSCALL_SEEK					equ 29
SYSCALL_STAT					equ 30
SYSCALL_DEBUG					equ 31
SYSCALL_CREATESHMEM		equ 32
SYSCALL_JOINSHMEM			equ 33
SYSCALL_LEAVESHMEM		equ 34
SYSCALL_DESTROYSHMEM	equ 35

; the IRQ for syscalls
SYSCALL_IRQ						equ	0x30

; macros for the different syscall-types (void / ret-value, arg-count, error-handling)

%macro SYSC_VOID_0ARGS 2
[global %1]
%1:
	mov		eax,%2								; set syscall-number
	int		SYSCALL_IRQ
	ret
%endmacro

%macro SYSC_VOID_1ARGS 2
[global %1]
%1:
	mov		eax,%2								; set syscall-number
	mov		ecx,[esp + 4]					; set arg1
	int		SYSCALL_IRQ
	ret
%endmacro

%macro SYSC_RET_0ARGS 2
[global %1]
%1:
	mov		eax,%2								; set syscall-number
	int		SYSCALL_IRQ
	ret													; return-value is in eax
%endmacro

%macro SYSC_RET_0ARGS_ERR 2
[global %1]
%1:
	mov		eax,%2								; set syscall-number
	int		SYSCALL_IRQ
	test	ecx,ecx
	jz		.return								; no-error?
	mov		[errno],ecx						; store error-code
	mov		eax,ecx								; return error-code
.return:
	ret
%endmacro

%macro SYSC_RET_1ARGS_ERR 2
[global %1]
%1:
	mov		eax,%2								; set syscall-number
	mov		ecx,[esp + 4]					; set arg1
	int		SYSCALL_IRQ
	test	ecx,ecx
	jz		.return								; no-error?
	mov		[errno],ecx						; store error-code
	mov		eax,ecx								; return error-code
.return:
	ret
%endmacro

%macro SYSC_RET_2ARGS_ERR 2
[global %1]
%1:
	mov		eax,%2								; set syscall-number
	mov		ecx,[esp + 4]					; set arg1
	mov		edx,[esp + 8]					; set arg2
	int		SYSCALL_IRQ
	test	ecx,ecx
	jz		.return								; no-error?
	mov		[errno],ecx						; store error-code
	mov		eax,ecx								; return error-code
.return:
	ret
%endmacro

%macro SYSC_RET_3ARGS_ERR 2
[global %1]
%1:
	push	ebp
	mov		ebp,esp
	mov		ecx,[ebp + 8]					; set arg1
	mov		edx,[ebp + 12]				; set arg2
	mov		eax,[ebp + 16]				; push arg3
	push	eax
	mov		eax,%2								; set syscall-number
	int		SYSCALL_IRQ
	add		esp,4									; remove arg3
	test	ecx,ecx
	jz		.return								; no-error?
	mov		[errno],ecx						; store error-code
	mov		eax,ecx								; return error-code
.return:
	leave
	ret
%endmacro
