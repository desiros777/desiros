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


#include <klibc.h>
#include <types.h>
#include <list.h>
#include <mm.h>
#include <physmem.h>
#include <kvmm_slab.h>
#include <debug.h>

/** The structure of a range of kernel-space virtual addresses */
struct kvmm_range
{
  __u32 base_vaddr;
  __u32 nb_pages;

  /* The slab owning this range, or NULL */
  struct kslab *slab;

  struct kvmm_range *prev, *next;
};

/** The ranges are SORTED in (strictly) ascending base addresses */
static struct kvmm_range *kvmm_free_range_list, *kvmm_used_range_list;

/** The slab cache for the kvmm ranges */
static struct kslab_cache *kvmm_range_cache;

/** Helper function to get the closest preceding or containing
    range for the given virtual address */
static struct kvmm_range *
get_closest_preceding_kvmm_range(struct kvmm_range *the_list,
				 __u32 vaddr)
{
  int nb_elements;
  struct kvmm_range *a_range, *ret_range;

  /* kvmm_range list is kept SORTED, so we exit as soon as vaddr >= a
     range base address */
  ret_range = NULL;
  list_foreach(the_list, a_range, nb_elements)
    {
      if (vaddr < a_range->base_vaddr)
	return ret_range;
      ret_range = a_range;
    }

  /* This will always be the LAST range in the kvmm area */
  return ret_range;
}


/**
 * function to lookup a free range large enough to hold nb_pages
 * pages (first fit)
 */
static struct kvmm_range *find_suitable_free_range(__u32 nb_pages)
{
  int nb_elements;
  struct kvmm_range *r;

  list_foreach(kvmm_free_range_list, r, nb_elements)
  {
    if (r->nb_pages >= nb_pages)
      return r;
  }

  debug();
  return NULL;
}

/**
 * Helper function to add a_range in the_list, in strictly ascending order.
 *
 * @return The (possibly) new head of the_list
 */
static struct kvmm_range *insert_range(struct kvmm_range *the_list,
					   struct kvmm_range *a_range)
{
  struct kvmm_range *prec_used;

  /** Look for any preceding range */
  prec_used = get_closest_preceding_kvmm_range(the_list,
					       a_range->base_vaddr);
  /** insert a_range /after/ this prec_used */
  if (prec_used != NULL)
    list_insert_after(the_list, prec_used, a_range);
  else /* Insert at the beginning of the list */
    list_add_head(the_list, a_range);

  return the_list;
}

/**
 * Function to retrieve the range owning the given vaddr, by
 * scanning the physical memory first if vaddr is mapped in RAM
 */
static struct kvmm_range *lookup_range(__u32 vaddr)
{
  struct kvmm_range *range;

  /* First: try to retrieve the physical page mapped at this address */
  __u32 ppage_paddr = PAGE_ALIGN_INF(paging_virtual_to_physical(page_directory,vaddr));

  if (ppage_paddr)
    {
      range = physmem_get_kvmm_range(ppage_paddr);

      /* If a page is mapped at this address, it is EXPECTED that it
	 is really associated with a range */
      if (range == NULL)
        debug();
    }

  /* Otherwise scan the list of used ranges, looking for the range
     owning the address */
  else
    {
      range = get_closest_preceding_kvmm_range(kvmm_used_range_list,
					       vaddr);
      /* Not found */
      if (! range)
	return NULL;

      /* vaddr not covered by this range */
      if ( (vaddr < range->base_vaddr)
	   || (vaddr >= (range->base_vaddr + range->nb_pages*PAGE_SIZE)) )
	return NULL;
    }

  return range;
}

/**
 * Helper function for kvmm_setup() to initialize a new range
 * that maps a given area as free or as already used.
 * This function either succeeds or halts the whole system.
 */
static struct kvmm_range *
kvmm_create_range(bool is_free,
	      __u32 base_vaddr,
	      __u32 top_vaddr,
	     struct kslab *associated_slab)
{
  struct kvmm_range *range;

  if(!IS_PAGE_ALIGNED(base_vaddr))
      debug();
  if(!IS_PAGE_ALIGNED(top_vaddr))
        debug();

  if ((top_vaddr - base_vaddr) < PAGE_SIZE)
    return NULL;
   
  range = (struct kvmm_range*) kvmm_cache_alloc(kvmm_range_cache,
						       KSLAB_ALLOC_ATOMIC);
  if(range == NULL)
     debug();

  range->base_vaddr = base_vaddr;
  range->nb_pages   = (top_vaddr - base_vaddr) / PAGE_SIZE;

  if (is_free)
    {
      list_add_tail(kvmm_free_range_list,
		    range);
    }
  else
    {
      __u32 vaddr;
      range->slab = associated_slab;
      list_add_tail(kvmm_used_range_list,
		    range);

      /* Ok, set the range owner for the pages in this page */
      for (vaddr = base_vaddr ;
	   vaddr < top_vaddr ;
	   vaddr += PAGE_SIZE)
      {
	__u32 ppage_paddr = paging_virtual_to_physical(page_directory ,vaddr);
	if((void*)ppage_paddr == NULL)
           debug();
	physmem_set_kvmm_range(ppage_paddr, range);
      }
    }

  return range;
}
/**
 * Allocate a new kernel area spanning one or multiple pages.
 *
 * @eturn a new range structure
 */
struct kvmm_range *kvmm_new_range(__u32 nb_pages,
					      __u32  flags,
					      __u32 * range_start)
{
  struct kvmm_range *free_range, *new_range;

  if (nb_pages <= 0){
     debug();
    return NULL;
    }
   
  

  /* Find a suitable free range to hold the size-sized object */
  free_range = find_suitable_free_range(nb_pages);
  if (free_range == NULL){
      debug();
    return NULL;
     }
    

  /* If range has exactly the requested size, just move it to the
     "used" list */
  if(free_range->nb_pages == nb_pages)
    {
      list_delete(kvmm_free_range_list, free_range);
      kvmm_used_range_list = insert_range(kvmm_used_range_list,
					  free_range);
      /* The new_range is exactly the free_range */
      new_range = free_range;
    }

  /* Otherwise the range is bigger than the requested size, split it.
     This involves reducing its size, and allocate a new range, which
     is going to be added to the "used" list */
  else
    {
      /* free_range split in { new_range | free_range } */
      new_range = (struct kvmm_range*)
	kvmm_cache_alloc(kvmm_range_cache,
			     (flags & KVMM_ATOMIC)?
			     KSLAB_ALLOC_ATOMIC:0);
      if (! new_range){
        debug();
	return NULL;
       }
      

      new_range->base_vaddr   = free_range->base_vaddr;
      new_range->nb_pages     = nb_pages;
      free_range->base_vaddr += nb_pages* PAGE_SIZE;
      free_range->nb_pages   -= nb_pages;

      /* free_range is still at the same place in the list */
      /* insert new_range in the used list */
      kvmm_used_range_list = insert_range(kvmm_used_range_list,
					  new_range);
    }

  /* By default, the range is not associated with any slab */
  new_range->slab = NULL;

  /* If mapping of physical pages is needed, map them now */
  if (flags & KVMM_MAP)
    {
      unsigned int i;
      for (i = 0 ; i < nb_pages ; i ++)
	{
	  /* Get a new physical page */
	  __u32 ppage_paddr
	    = physmem_ref_physpage_new(false);
	  
	  /* Map the page in kernel space */
	  if (ppage_paddr)
	    {
              if(paging_unmap(new_range->base_vaddr + i * PAGE_SIZE))
                     debug();
                
	      if (paging_map(new_range->base_vaddr + i * PAGE_SIZE, ppage_paddr
				 ,false ))
		{
		  /* Failed => force unallocation, see below */
		 physmem_unref_physpage(ppage_paddr);
		  ppage_paddr = (__u32)NULL;
		}
	      else
		{
		  /* Success : page can be unreferenced since it is
		     now mapped */
		physmem_unref_physpage(ppage_paddr);
		}
	    }

	  /* Undo the allocation if failed to allocate or map a new page */
	  if (! ppage_paddr)
	    {
              debug();
	      kvmm_del_range(new_range);
	      return NULL;
	    }

	  /* Ok, set the range owner for this page */
	  physmem_set_kvmm_range(ppage_paddr, new_range);
	}
    }
  /* ... Otherwise: Demand Paging will do the job */

  if (range_start)
    *range_start = new_range->base_vaddr;

  return new_range;


}


int kvmm_del_range(struct kvmm_range *range)
{
  struct kvmm_range *ranges_to_free;
  list_init(ranges_to_free);


  if(range == NULL)
   debug();
  if(range->slab != NULL)     
     debug();

  /* Remove the range from the 'USED' list now */
  list_delete(kvmm_used_range_list, range);

  /*
   * The following do..while() loop is here to avoid an indirect
   * recursion: if we call directly kvmm_cache_free() from inside the
   * current function, we take the risk to re-enter the current function
   * (kvmm_del_range()) again, which may cause problem if it
   * in turn calls kvmm_slab again and kvmm_del_range again,
   * and again and again. This may happen while freeing ranges of
   * struct kslab...
   *
   * To avoid this,we choose to call a special function of kvmm_slab
   * doing almost the same as kvmm_cache_free(), but which does
   * NOT call us (ie kvmm_del_range()): instead WE add the
   * range that is to be freed to a list, and the do..while() loop is
   * here to process this list ! The recursion is replaced by
   * classical iterations.
   */
  do
    {
      unsigned int i;

      /* Ok, we got the range. Now, insert this range in the free list */
      kvmm_free_range_list = insert_range(kvmm_free_range_list, range);

      /* Unmap the physical pages */
      for (i = 0 ; i < range->nb_pages ; i ++)
	{
	  /* This will work even if no page is mapped at this address */
	  paging_unmap(range->base_vaddr + i*PAGE_SIZE);
	}
      
      /* Eventually coalesce it with prev/next free ranges (there is
	 always a valid prev/next link since the list is circular). Note:
	 the tests below will lead to correct behaviour even if the list
	 is limited to the 'range' singleton, at least as long as the
	 range is not zero-sized */
      /* Merge with preceding one ? */
      if (range->prev->base_vaddr + range->prev->nb_pages*PAGE_SIZE
	  == range->base_vaddr)
	{
	  struct kvmm_range *empty_range_of_ranges = NULL;
	  struct kvmm_range *prec_free = range->prev;
	  
	  /* Merge them */
	  prec_free->nb_pages += range->nb_pages;
	  list_delete(kvmm_free_range_list, range);
	  
	  /* Mark the range as free. This may cause the slab owning
	     the range to become empty */
	  empty_range_of_ranges = 
	    kvmm_cache_release_struct_range(range);

	  /* If this causes the slab owning the range to become empty,
	     add the range corresponding to the slab at the end of the
	     list of the ranges to be freed: it will be actually freed
	     in one of the next iterations of the do{} loop. */
	  if (empty_range_of_ranges != NULL)
	    {
	      list_delete(kvmm_used_range_list, empty_range_of_ranges);
	      list_add_tail(ranges_to_free, empty_range_of_ranges);
	    }
	  
	  /* Set range to the beginning of this coelescion */
	  range = prec_free;
	}
      
      /* Merge with next one ? [NO 'else' since range may be the result of
	 the merge above] */
      if (range->base_vaddr + range->nb_pages*PAGE_SIZE
	  == range->next->base_vaddr)
	{
	  struct kvmm_range *empty_range_of_ranges = NULL;
	  struct kvmm_range *next_range = range->next;
	  
	  /* Merge them */
	  range->nb_pages += next_range->nb_pages;
	  list_delete(kvmm_free_range_list, next_range);
	  
	  /* Mark the next_range as free. This may cause the slab
	     owning the next_range to become empty */
	  empty_range_of_ranges =  (struct kvmm_range*) kvmm_cache_release_struct_range(next_range);

	  /* If this causes the slab owning the next_range to become
	     empty, add the range corresponding to the slab at the end
	     of the list of the ranges to be freed: it will be
	     actually freed in one of the next iterations of the
	     do{} loop. */
	  if (empty_range_of_ranges != NULL)
	    {
	      list_delete(kvmm_used_range_list, empty_range_of_ranges);
	      list_add_tail(ranges_to_free, empty_range_of_ranges);
	    }
	}
      

      /* If deleting the range(s) caused one or more range(s) to be
	 freed, get the next one to free */
      if (list_is_empty(ranges_to_free))
	range = NULL; /* No range left to free */
      else
	range = list_pop_head(ranges_to_free);

    }
  /* Stop when there is no range left to be freed for now */
  while (range != NULL);

  return 0;
}


struct kslab * kvmm_resolve_slab(__u32 vaddr)
{
  struct kvmm_range *range = lookup_range(vaddr);
  if (! range){
     debug();
    return NULL;
    }
  return range->slab;
}



__u32 kvmm_alloc(__u32 nb_pages, __u32  flags)
{
  struct kvmm_range *range
    = kvmm_new_range(nb_pages,
			     flags,
			     NULL);
  if (! range)
    return (__u32)NULL;
  
  return range->base_vaddr;
}


int kvmm_free(__u32 vaddr)
{
  struct kvmm_range *range = lookup_range(vaddr);

  /* We expect that the given address is the base address of the
     range */
  if (!range || (range->base_vaddr != vaddr))
    return -1;

  /* We expect that this range is not held by any cache */
  if (range->slab != NULL)
    return -4;

  return kvmm_del_range(range);
}



int kvmm_set_slab(struct kvmm_range *range,
				struct kslab *slab)
{
  if (! range)
    return -3;

  range->slab = slab;
  return 0;
}

int kvmm_setup(__u32  kernel_core_base,
			     __u32  kernel_core_top,
			     __u32  stack_bottom_vaddr,
			     __u32  stack_top_vaddr)
{


    struct kslab *first_struct_slab_of_caches,
    *first_struct_slab_of_ranges;

   __u32 first_slab_of_caches_base,
    first_slab_of_caches_nb_pages,
    first_slab_of_ranges_base,
    first_slab_of_ranges_nb_pages;

struct kvmm_range *first_range_of_caches,
    *first_range_of_ranges;



  list_init(kvmm_free_range_list);
  list_init(kvmm_used_range_list);

  


 kvmm_range_cache = kvmm_cache_setup_prepare(kernel_core_base,
					     kernel_core_top,
					     sizeof(struct kvmm_range),
                                             & first_struct_slab_of_caches,
					     & first_slab_of_caches_base,
					     & first_slab_of_caches_nb_pages,
                                             & first_struct_slab_of_ranges,
					     & first_slab_of_ranges_base,
					     & first_slab_of_ranges_nb_pages);


  
  if(kvmm_range_cache == NULL)
    return -1 ;
   
   



  /* Mark virtual addresses 16kB - Video as FREE */
  kvmm_create_range(true,
	       KVMM_BASE,
	       PAGE_ALIGN_INF(BIOS_N_VIDEO_START),
	       NULL);
  
  /* Mark virtual addresses in Video hardware mapping as NOT FREE */
  kvmm_create_range(false,
	       PAGE_ALIGN_INF(BIOS_N_VIDEO_START),
	       PAGE_ALIGN_SUP(BIOS_N_VIDEO_END),
	       NULL);
  
  /* Mark virtual addresses Video - Kernel as FREE */
  kvmm_create_range(true,
	       PAGE_ALIGN_SUP(BIOS_N_VIDEO_END),
	       PAGE_ALIGN_INF(kernel_core_base),
	       NULL);
  
  /* Mark virtual addresses in Kernel code/data up to the bootstrap stack
     as NOT FREE */
  kvmm_create_range(false,
	       PAGE_ALIGN_INF(kernel_core_base),
	       stack_bottom_vaddr,
	       NULL);

  /* Mark virtual addresses in the bootstrap stack as NOT FREE too,
     but in another vmm region in order to be un-allocated later */
  kvmm_create_range(false,
	       stack_bottom_vaddr,
	       stack_top_vaddr,
	       NULL);

  /* Mark the remaining virtual addresses in Kernel code/data after
     the bootstrap stack as NOT FREE */
  kvmm_create_range(false,
	       stack_top_vaddr,
	       first_slab_of_ranges_base,
	       NULL);

  /* Mark virtual addresses in the first slab of the cache of caches
     as NOT FREE */
  if(PAGE_ALIGN_SUP(kernel_core_top) != first_slab_of_caches_base){
      debug();
    return -1 ;
   }

   first_range_of_caches
    =  kvmm_create_range(false,
		   first_slab_of_caches_base,
		   first_slab_of_caches_base
		   + first_slab_of_caches_nb_pages*PAGE_SIZE,
		   first_struct_slab_of_caches);



   /* Mark virtual addresses in the first slab of the cache of ranges
     as NOT FREE */
  if((first_slab_of_caches_base
		    + first_slab_of_caches_nb_pages*PAGE_SIZE)
		   != first_slab_of_ranges_base)
                     debug();
               
    first_range_of_ranges
    =  kvmm_create_range(false,
		   first_slab_of_ranges_base,
		   first_slab_of_ranges_base
		   + first_slab_of_ranges_nb_pages*PAGE_SIZE,
		   first_struct_slab_of_ranges);

 
  
  /* Mark virtual addresses after these slabs as FREE */
  kvmm_create_range(true,
	       first_slab_of_ranges_base
	       + first_slab_of_ranges_nb_pages*PAGE_SIZE,
	       KVMM_TOP,
	       NULL);






  return 0;
}

