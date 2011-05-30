#
# $Id$
# Copyright (C) 2008 - 2009 Nils Asmussen
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
#

.section .text

.global _start
.global sigRetFunc

.include "../../../lib/c/arch/i586/syscalls.s"

_start:
	# load modules first
	mov		$SYSCALL_LOADMODS,%eax
	int		$SYSCALL_IRQ

	# now replace with init
	mov		$progName,%ecx						# set path
	mov		$args,%edx								# set arguments
	mov		$SYSCALL_EXEC,%eax				# set syscall-number
	int		$SYSCALL_IRQ

	# we should not reach this
1:
	jmp		1b

# provide just a dummy
sigRetFunc:
	jmp		sigRetFunc

args:
	.long			progName,0

progName:
	.asciz		"/bin/init"