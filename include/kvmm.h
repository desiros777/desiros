/* Copyright (C) 2004,2005  The DESIROS Team
    desiros.dev@gmail.com

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307,
   USA. 
 */


#ifndef _KVMM_H_
#define _KVMM_H_


struct kvmm_range;

#include <kvmm_slab.h>

/**
 * Mark the areas belonging to KVMM_BASE and _KVMM_TOP
 * are either used or free. Those that are already mapped are marked
 * as "used", and the 0..KVMM_BASE virtual addresses as marked
 * as "used" too (to detect incorrect pointer dereferences).
 */
int kvmm_setup(__u32  kernel_core_base,
			     __u32  kernel_core_top,
			     __u32  stack_bottom_vaddr,
			     __u32  stack_top_vaddr);




/**
 * Allocate a new kernel area spanning one or multiple pages.
 *
 * @param range_base_vaddr If not NULL, the start address of the range
 * is stored in this location
 * @eturn a new range structure
 */
struct kvmm_range *kvmm_new_range(__u32 nb_pages,
					      __u32  flags,
					      __u32 * range_start);

int kvmm_del_range(struct kvmm_range *range);


/**
 * Straighforward variant of kvmm_new_range() returning the
 * range's start address instead of the range structure
 */
__u32 kvmm_alloc(__u32 nb_pages,__u32 flags);

/**
 * @note you are perfectly allowed to give the address of the
 * kernel image, or the address of the bios area here, it will work:
 * the kernel/bios WILL be "deallocated". But if you really want to do
 * this, well..., do expect some "surprises" ;)
 */
int kvmm_free(__u32 vaddr);


/**
 * @return TRUE when vaddr is covered by any (used) kernel range
 */
bool kvmm_is_valid_vaddr(__u32 vaddr);


/* *****************************
 * Reserved to kvmm_slab.c ONLY.
 */
/**
 * Associate the range with the given slab.
 */
int kvmm_set_slab(struct kvmm_range *range,
				struct kslab *slab);

/**
 * Retrieve the (used) slab associated with the range covering vaddr.
 *
 * @return NULL if the range is not associated with a KVMM range
 */
struct kslab *kvmm_resolve_slab(__u32  vaddr);

#endif /* _KVMM_H_ */
