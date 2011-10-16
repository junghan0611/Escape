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

#ifndef SWAP_H_
#define SWAP_H_

#include <sys/common.h>
#include <sys/task/proc.h>

/**
 * Starts the swapping-system. This HAS TO be done with the swapping-thread!
 */
void swap_start(void);

/**
 * Swaps the page with given address for the current process in
 *
 * @param addr the address of the page
 * @return true if successfull
 */
bool swap_in(uintptr_t addr);

/**
 * Swaps out frames until at least <frameCount> frames are available.
 * Panics if its not possible to make that frames available (swapping disabled, partition full, ...)
 *
 * @param frameCount the number of frames you need
 */
void swap_reserve(size_t frameCount);

/**
 * Allocates one frame. Assumes that it is available. You should announce it with swap_reserve()
 * first!
 *
 * @param critical whether to allocate critical memory
 * @return the frame-number or 0 if no free frame is available
 */
frameno_t swap_allocate(bool critical);

/**
 * Frees the given frame
 *
 * @param frame the frame-number
 * @param critical whether this frame has been used for critical memory
 */
void swap_free(frameno_t frame,bool critical);

#endif /* SWAP_H_ */
