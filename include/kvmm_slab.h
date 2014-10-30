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


#ifndef _KVMM_SLAB_H_
#define _KVMM_SLAB_H_

#include <mm.h>

struct kslab_cache;


struct kslab;




/** The maximum allowed pages for each slab */
#define MAX_PAGES_PER_SLAB 32 /* 128 kB */


struct kslab_cache *
kvmm_cache_setup_prepare(__u32  kernel_core_base,
				       __u32  kernel_core_top,
				       __u32   sizeof_struct_range,
                                       struct kslab **first_struct_slab_of_caches,
				       __u32  *first_slab_of_caches_base,
				       __u32  *first_slab_of_caches_nb_pages,
                                        struct kslab **first_struct_slab_of_ranges,
				       __u32  *first_slab_of_ranges_base,
				       __u32  *first_slab_of_ranges_nb_pages);





#define KSLAB_CREATE_MAP  (1<<0)
/** The object should always be set to zero at allocation (implies
   KSLAB_CREATE_MAP) */
#define KSLAB_CREATE_ZERO (1<<1)

struct kslab_cache *kvmm_cache_create(const char* name,
		      __u32  obj_size,
		      __u32 pages_per_slab,
		      __u32 min_free_objs,
		      __u32 cache_flags);

int kvmm_cache_destroy(struct kslab_cache *kslab_cache_p);



/** Allocation should either succeed or fail, without blocking */
#define KSLAB_ALLOC_ATOMIC (1<<0)


__u32 kvmm_cache_alloc(struct kslab_cache *kslab_cache_p,
				 __u32 alloc_flags);


int kvmm_cache_free(__u32 vaddr);


struct kvmm_range *kvmm_cache_release_struct_range(struct kvmm_range *the_range);

#endif

