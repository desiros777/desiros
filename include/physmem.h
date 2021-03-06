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



#ifndef _PHYSMEM_H_
#define _PHYSMEM_H_

/*
 * Physical pages of memory
 */

#include <types.h>
#include <macros.h>
#include <kvmm.h>

/** The size of a physical page (arch-dependent) */
#define PAGE_SIZE  4096

/** The corresponding shift */
#define PAGE_SHIFT 12 /* 4 kB = 2^12 B */

/** The corresponding mask */
#define PAGE_MASK  ((1<<12) - 1)

#define PAGE_ALIGN_INF(val)  \
  ALIGN_INF((val), PAGE_SIZE)
#define PAGE_ALIGN_SUP(val)  \
  ALIGN_SUP((val), PAGE_SIZE)
#define IS_PAGE_ALIGNED(val) \
  IS_ALIGNED((val), PAGE_SIZE)

/**
 * This is the reserved physical interval for the x86 video memory and
 * BIOS area. In physmem.c, we have to mark this area as "nonfree" in
 * order to prevent from allocating it. And in paging.c, we'd better
 * map it in virtual space if we really want to be able to print to
 * the screen (for debugging purpose, at least): for this, the
 * simplest is to identity-map this area in virtual space (note
 * however that this mapping could also be non-identical).
 */
#define BIOS_N_VIDEO_START 0xa0000
#define BIOS_N_VIDEO_END   0x100000


/**
 * Initialize the physical memory subsystem, for the physical area [0,
 * ram_size). This routine takes into account the BIOS and video
 * areas, to prevent them from future allocations.
 *
 * @param ram_size The size of the RAM that will be managed by this subsystem
 *
 * @param kernel_core_base The lowest address for which the kernel
 * assumes identity mapping (ie virtual address == physical address)
 * will be stored here
 *
 * @param kernel_core_top The top address for which the kernel
 * assumes identity mapping (ie virtual address == physical address)
 * will be stored here
 */
int physmem_setup(size_t ram_size,
				      /* out */__u32 *kernel_core_base,
				      /* out */__u32 *kernel_core_top);


/**
 * Retrieve the total number of pages, and the number of free pages
 *
 * @note both parameters may be NULL
 */
int physmem_get_state(/* out */__u32 *total_ppages,
				/* out */__u32 *nonfree_ppages);


/**
 * Get a free page.
 *
 * @return The (physical) address of the (physical) page allocated, or
 * NULL when none currently available.
 *
 * @param can_block TRUE if the function is allowed to block
 * @note The page returned has a reference count equal to 1.
 */
__u32 physmem_ref_physpage_new(bool can_block);


/**
 * Increment the reference count of a given physical page. Useful for
 * VM code which tries to map a precise physical address.
 *
 * @param ppage_paddr Physical address of the page (MUST be page-aligned)
 *
 * @return TRUE when the page was previously referenced, FALSE when
 * the page was previously unreferenced, <0 when the page address is
 * invalid.
 */
int physmem_ref_physpage_at(__u32 ppage_paddr);


/**
 * Decrement the reference count of the given physical page. When the
 * reference count of the page reaches 0, the page is marked free, ie
 * is available for future physmem_ref_physpage_new()
 *
 * @param ppage_paddr Physical address of the page (MUST be page-aligned)
 *
 * @return FALSE when the page is still referenced, TRUE when the page
 * is now unreferenced, <0 when the page address is invalid
 */
int physmem_unref_physpage(__u32 ppage_paddr);


/**
 * Return the reference count of the given page
 *
 * @return >= 0 (the referebce count of the page) if the physical
 * address is valid, or an error status
 */
int physmem_get_physpage_refcount(__u32 ppage_paddr);



struct kvmm_range* physmem_get_kvmm_range(__u32 ppage_paddr);

int physmem_set_kvmm_range(__u32 ppage_paddr,
				     struct kvmm_range *range);




#endif /* _PHYSMEM_H_ */
