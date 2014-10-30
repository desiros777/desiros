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
#include <debug.h>
#include <kvmm.h>
/* Dimensioning constants */
#define NB_PAGES_IN_SLAB_OF_CACHES 1
#define NB_PAGES_IN_SLAB_OF_RANGES 1

/** The structure of a slab cache */
struct kslab_cache
{
  char const* name;

  /* non mutable characteristics of this slab */
 __u32   original_obj_size; /* asked object size */
 __u32   alloc_obj_size;    /* actual object size, taking the
				    alignment constraints into account */
  __u32  nb_obj_per_slab;
  __u32  nb_pages_per_slab;
  __u32  min_free_obj;


#define ON_SLAB (1<<31) /* struct kslab  is included inside the slab */
  __u32  flags;

  /* Supervision data (updated at run-time) */
  __u32  nb_free_obj;

  /* The lists of slabs owned by this cache */
  struct  kslab *slab_list; /* head = non full, tail = full */

  /* The caches are linked together on the kslab_cache_list */
  struct  kslab_cache *prev, *next;
};


/** The structure of a slab */
struct kslab
{
  /** Number of free objects on this slab */
  __u32 nb_free;

  /** The list of these free objects */
  struct kslab_free_obj *free;

  /** The address of the associated range structure */
  struct kvmm_range *range;

  /** Virtual start address of this range */
  __u32 first_obj;
  
  /** Slab cache owning this slab */
  struct  kslab_cache *cache;

  /** Links to the other slabs managed by the same cache */
  struct kslab *prev, *next;
};

/** The structure of the free objects in the slab */
struct kslab_free_obj
{
  struct kslab_free_obj *prev, *next;
};

/** The cache of slab caches */
struct kslab_cache *cache_of_struct_kslab_cache;

/** The cache of slab structures for non-ON_SLAB caches */
static struct kslab_cache *cache_of_struct_kslab;

/** The list of slab caches */
static struct kslab_cache *kslab_cache_list;

/* Helper function to initialize a cache structure */
static int
init_cache(/*out*/struct kslab_cache *the_cache,
		 const char* name,
		 __u32 obj_size,
		 __u32 pages_per_slab,
		 __u32 min_free_objs,
		 __u32 cache_flags)
{
  unsigned int space_left;
  __u32 alloc_obj_size;

  if (obj_size <= 0)
    return -1;

  /* Default allocation size is the requested one */
  alloc_obj_size = obj_size;

  /* Make sure the requested size is large enough to store a
     free_object structure */
  if (alloc_obj_size < sizeof(struct kslab_free_obj))
    alloc_obj_size = sizeof(struct kslab_free_obj);
  
  /* Align obj_size on 4 bytes */
  alloc_obj_size = ALIGN_SUP(alloc_obj_size, sizeof(int));

  /* Make sure supplied number of pages per slab is consistent with
     actual allocated object size */
  if (alloc_obj_size > pages_per_slab*PAGE_SIZE)
    return -1;
  
  /* Refuse too large slabs */
  if (pages_per_slab > MAX_PAGES_PER_SLAB)
    return -3;

  /* Fills in the cache structure */
  memset(the_cache, 0x0, sizeof(struct kslab_cache));
  the_cache->name              = name;
  the_cache->flags             = cache_flags;
  the_cache->original_obj_size = obj_size;
  the_cache->alloc_obj_size    = alloc_obj_size;
  the_cache->min_free_obj  = min_free_objs;
  the_cache->nb_pages_per_slab = pages_per_slab;
  
  /* Small size objets => the slab structure is allocated directly in
     the slab */
  if(alloc_obj_size <= sizeof(struct kslab))
    the_cache->flags |= ON_SLAB;
  
  /*
   * Compute the space left once the maximum number of objects
   * have been allocated in the slab
   */
  space_left = the_cache->nb_pages_per_slab*PAGE_SIZE;
  if(the_cache->flags & ON_SLAB)
    space_left -= sizeof(struct kslab);
  the_cache->nb_obj_per_slab = space_left / alloc_obj_size;
  space_left -= the_cache->nb_obj_per_slab*alloc_obj_size;

  /* Make sure a single slab is large enough to contain the minimum
     number of objects requested */
  if (the_cache->nb_obj_per_slab < min_free_objs)
    return -1;

  /* If there is now enough place for both the objects and the slab
     structure, then make the slab structure ON_SLAB */
  if (space_left >= sizeof(struct kslab))
    the_cache->flags |= ON_SLAB;

  return 0 ;
}

/**
 * Helper function to release a slab
 *
 * The corresponding range is always deleted, except when the @param
 * must_del_range_now is not set. This happens only when the function
 * gets called from kvmm_cache_release_struct_range(), to avoid
 * large recursions.
 */
static int
cache_release_slab(struct kslab *slab,
		   bool must_del_range_now)
{
  struct kslab_cache *kslab_cache_p = slab->cache;
  struct kvmm_range *range = slab->range;

  if(kslab_cache_p == NULL)
  debug();

  if(range == NULL)
  debug();

  if(slab->nb_free != slab->cache->nb_obj_per_slab)
    debug();

  /* First, remove the slab from the slabs' list of the cache */
  list_delete(kslab_cache_p->slab_list, slab);
  slab->cache->nb_free_obj -= slab->nb_free;

  /* Release the slab structure if it is OFF slab */
  if (! (slab->cache->flags & ON_SLAB))
    kvmm_cache_free((__u32 )slab);

  /* Ok, the range is not bound to any slab anymore */
  kvmm_set_slab(range, NULL);

  /* Always delete the range now, unless we are told not to do so (see
     kvmm_cache_release_struct_range() below) */
  if (must_del_range_now)
    return kvmm_del_range(range);

  return 0 ;
}

inline static int free_object(__u32 vaddr,
	    struct kslab ** empty_slab)
{
  struct kslab_cache *kslab_cache_p;

  /* Lookup the slab containing the object in the slabs' list */
  struct kslab *slab = (struct kslab*) kvmm_resolve_slab(vaddr);

  /* By default, consider that the slab will not become empty */
  *empty_slab = NULL;

  /* Did not find the slab */
  if (! slab)
    return -3;

  if(!slab->cache)
  debug();

  kslab_cache_p = slab->cache;

  /*
   * Check whether the address really could mark the start of an actual
   * allocated object
   */
  /* Address multiple of an object's size ? */
  if (( (vaddr - slab->first_obj)
	% kslab_cache_p->alloc_obj_size) != 0)
    return -3;
  /* Address not too large ? */
  if (( (vaddr - slab->first_obj)
	/ kslab_cache_p->alloc_obj_size) >= kslab_cache_p->nb_obj_per_slab)
    return -3;

  /*
   * Ok: we now release the object
   */

  /* Did find a full slab => will not be full any more => move it
     to the head of the slabs' list */
  if (! slab->free)
    {
      list_delete(kslab_cache_p->slab_list, slab);
      list_add_head(kslab_cache_p->slab_list, slab);
    }

  /* Release the object */
  list_add_head(slab->free, (struct kslab_free_obj*)vaddr);
  slab->nb_free++;
  kslab_cache_p->nb_free_obj++;
  //if(slab->nb_free >= slab->cache->nb_obj_per_slab)
  // debug();

  /* Cause the slab to be released if it becomes empty, and if we are
     allowed to do it */
  if ((slab->nb_free >= kslab_cache_p->nb_obj_per_slab)
      && (kslab_cache_p->nb_free_obj - slab->nb_free
	  >= kslab_cache_p->min_free_obj))
    {
      *empty_slab = slab;
    }

  return 0;
}

/** Helper function to add a new slab for the given cache. */
static int   
cache_add_slab(struct kslab_cache *kslab_cache_p,
	       __u32 vaddr_slab,
	       struct kslab *slab)
{
  unsigned int i;

  /* Setup the slab structure */
  memset(slab, 0x0, sizeof(struct kslab));
  slab->cache = kslab_cache_p;

  /* Establish the address of the first free object */
  slab->first_obj = vaddr_slab;

  /* Account for this new slab in the cache */
  slab->nb_free = kslab_cache_p->nb_obj_per_slab;
  kslab_cache_p->nb_free_obj += slab->nb_free;

  /* Build the list of free objects */
  for (i = 0 ; i <  kslab_cache_p->nb_obj_per_slab ; i++)
    {
      __u32 obj_vaddr;

      /* Set object's address */
      obj_vaddr = slab->first_obj + i*kslab_cache_p->alloc_obj_size;

      /* Add it to the list of free objects */
      list_add_tail(slab->free,
		    (struct kslab_free_obj *)obj_vaddr);
    }

  /* Add the slab to the cache's slab list: add the head of the list
     since this slab is non full */
  list_add_head(kslab_cache_p->slab_list, slab);

  return 0;
}



int kvmm_cache_free(__u32 vaddr)
{
  __u32 retval;
  struct kslab *empty_slab;

  /* Remove the object from the slab */
  retval = free_object(vaddr, & empty_slab);
  if (retval != 0)
    return retval;

  /* Remove the slab and the underlying range if needed */
  if (empty_slab != NULL)
    return cache_release_slab(empty_slab, true);

  return 0;
}




struct kvmm_range *kvmm_cache_release_struct_range(struct kvmm_range *the_range)
{
  __u32 retval;
  struct kslab *empty_slab;

  /* Remove the object from the slab */
  retval = free_object((__u32)the_range, & empty_slab);
  if (retval != 0)
    return NULL;

  /* Remove the slab BUT NOT the underlying range if needed */
  if (empty_slab != NULL)
    {
      struct kvmm_range *empty_range = empty_slab->range;
      if(cache_release_slab(empty_slab, false) != 0)
     debug();

      if(empty_range == NULL)
       debug();

      return empty_range;
    }

  return NULL;
}

#include <kvmm_slab.h>

/** Helper function to allocate a new slab for the given kslab_cache */
static int   
cache_grow(struct kslab_cache *kslab_cache_p,
	   __u32 alloc_flags)
{
  __u32  range_alloc_flags;

  struct kvmm_range *new_range;
  __u32  new_range_start;

  struct kslab *new_slab;

  /*
   * Setup the flags for the range allocation
   */
  range_alloc_flags = 0;

  /* Atomic ? */
  if (alloc_flags & KSLAB_ALLOC_ATOMIC)
    range_alloc_flags |= KVMM_ATOMIC;

  /* Need physical mapping NOW ? */
  if (kslab_cache_p->flags & (KSLAB_CREATE_MAP
			   | KSLAB_CREATE_ZERO))
    range_alloc_flags |= KVMM_MAP;



  /* Allocate the range */
  new_range = (struct kvmm_range *) kvmm_new_range(kslab_cache_p->nb_pages_per_slab,
				     range_alloc_flags,
				     & new_range_start);

 

  if (! new_range){
     debug();
    return -3;
   }
  

  /* Allocate the slab structure */
  if (kslab_cache_p->flags & ON_SLAB)
    {
      /* Slab structure is ON the slab: simply set its address to the
	 end of the range */
      __u32 slab_vaddr
	= new_range_start + kslab_cache_p->nb_pages_per_slab*PAGE_SIZE
	  - sizeof(struct kslab);
      new_slab = (struct kslab*)slab_vaddr;
    }
  else
    {
      /* Slab structure is OFF the slab: allocate it from the cache of
	 slab structures */
      __u32 slab_vaddr
	= kvmm_cache_alloc(cache_of_struct_kslab,
			       alloc_flags);
      if (! slab_vaddr)
	{
          debug();
	  kvmm_del_range(new_range);
	  return -3;
	}
      new_slab = (struct kslab *)slab_vaddr;
    }

  cache_add_slab(kslab_cache_p, new_range_start, new_slab);
  new_slab->range = new_range;

  /* Set the backlink from range to this slab */
  kvmm_set_slab(new_range, new_slab);

  return 0;
}

int kvmm_cache_destroy(struct kslab_cache *kslab_cache_p)
{
  int nb_slabs;
  struct kslab *slab;

  if (! kslab_cache_p)
    return -1;

  /* Refuse to destroy the cache if there are any objects still
     allocated */
  list_foreach(kslab_cache_p->slab_list, slab, nb_slabs)
    {
      if (slab->nb_free != kslab_cache_p->nb_obj_per_slab)
	return -4;
    }

  /* Remove all the slabs */
  while ((slab = list_get_head(kslab_cache_p->slab_list)) != NULL)
    {
      cache_release_slab(slab, true);
    }

  /* Remove the cache */
  return kvmm_cache_free((__u32)kslab_cache_p);
}



__u32 kvmm_cache_alloc(struct kslab_cache *kslab_cache_p,
				 __u32 alloc_flags)
{
  __u32 obj_vaddr;
  struct kslab * slab_head;

 if(kslab_cache_p->nb_pages_per_slab == 0)
              debug();


  /* If the slab at the head of the slabs' list has no free object,
     then the other slabs don't either => need to allocate a new
     slab */
  if ((! kslab_cache_p->slab_list)
      || (! list_get_head(kslab_cache_p->slab_list)->free))
    {
          
      if (cache_grow(kslab_cache_p, alloc_flags) != 0){
	/* Not enough memory or blocking alloc */
        debug();
	return (__u32)NULL;
        }
    }



  /* Here: we are sure that list_get_head(kslab_cache_p->slab_list)
     exists *AND* that list_get_head(kslab_cache_p->slab_list)->free is
     NOT NULL */
  slab_head = list_get_head(kslab_cache_p->slab_list);
  if(slab_head == NULL)
   debug();

  /* Allocate the object at the head of the slab at the head of the
     slabs' list */
  obj_vaddr = (__u32)list_pop_head(slab_head->free);
  slab_head->nb_free --;
  kslab_cache_p->nb_free_obj --;



  /* If needed, reset object's contents */
  if (kslab_cache_p->flags & KSLAB_CREATE_ZERO)
    memset((void*)obj_vaddr, 0x0, kslab_cache_p->alloc_obj_size);

  /* Slab is now full ? */
  if (slab_head->free == NULL)
    {
      /* Transfer it at the tail of the slabs' list */
      struct kslab *slab;
      slab = list_pop_head(kslab_cache_p->slab_list);
      list_add_tail(kslab_cache_p->slab_list, slab);
    }
  
  /*
   * For caches that require a minimum amount of free objects left,
   * allocate a slab if needed.
   *
   * Notice the "== min_objects - 1": we did not write " <
   * min_objects" because for the cache of kvmm structure, this would
   * lead to an chicken-and-egg problem, since cache_grow below would
   * call cache_alloc again for the kvmm cache, so we return here
   * with the same cache. If the test were " < min_objects", then we
   * would call cache_grow again for the kvmm cache again and
   * again... until we reach the bottom of our stack (infinite
   * recursion). By telling precisely "==", then the cache_grow would
   * only be called the first time.
   */


  if ((kslab_cache_p->min_free_obj > 0)
      && (kslab_cache_p->nb_free_obj == (kslab_cache_p->min_free_obj - 1)))
    {
      /* No: allocate a new slab now */
      if (cache_grow(kslab_cache_p, alloc_flags) != 0 )
	{
           debug();
	  /* Not enough free memory or blocking alloc => undo the
	     allocation */
	  kvmm_cache_free(obj_vaddr);
	  return (__u32)NULL;
	}
    }



   return obj_vaddr;
}


struct kslab_cache *kvmm_cache_create(const char* name,
		      __u32  obj_size,
		      __u32 pages_per_slab,
		      __u32 min_free_objs,
		      __u32 cache_flags)
{
  struct kslab_cache *new_cache;

  if(obj_size < 0)
     debug();

if(cache_of_struct_kslab_cache->nb_pages_per_slab == 0)
              debug();

  /* Allocate the new cache */
  new_cache = (struct kslab_cache*)
    kvmm_cache_alloc(cache_of_struct_kslab_cache,
			 0/* NOT ATOMIC */);
   
  if (! new_cache){
     debug();
    return NULL;
    }


  if (init_cache(new_cache, name, obj_size,
		       pages_per_slab, min_free_objs,
		       cache_flags))
    {
      /* Something was wrong */
      kvmm_cache_free((__u32)new_cache);
      debug();
      return NULL;
    }

     
  /* Add the cache to the list of slab caches */
  list_add_tail(kslab_cache_list, new_cache);


  /* if the min_free_objs is set, pre-allocate a slab */
  if (min_free_objs)
    {
      if (cache_grow(new_cache, 0 /* Not atomic */) != 0)
	{
	  kvmm_cache_destroy(new_cache);
          debug();
	  return NULL; /* Not enough memory */
	}
    }

  return new_cache;  
}

/**
 * Helper function to create the initial cache of caches, with a very
 * first slab in it, so that new cache structures can be simply allocated.
 * @return the cache structure for the cache of caches
 */
static struct kslab_cache *
create_cache_of_caches(__u32 vaddr_first_slab_of_caches,
		       int nb_pages)
{
  /* The preliminary cache structure we need in order to allocate the
     first slab in the cache of caches (allocated on the stack !) */
  struct kslab_cache fake_cache_of_caches;

  /* The real cache structure for the cache of caches */
  struct kslab_cache *real_cache_of_caches;

  /* The kslab structure for this very first slab */
  struct kslab       *slab_of_caches;

  /* Init the cache structure for the cache of caches */
  if (init_cache(& fake_cache_of_caches,
		       "Caches", sizeof(struct kslab_cache),
		       nb_pages, 0, KSLAB_CREATE_MAP | ON_SLAB))
    /* Something wrong with the parameters */
    return NULL;

  memset((void*)vaddr_first_slab_of_caches, 0x0, nb_pages*PAGE_SIZE);

  /* Add the pages for the 1st slab of caches */
  slab_of_caches = (struct kslab*)(vaddr_first_slab_of_caches
				       + nb_pages*PAGE_SIZE
				       - sizeof(struct kslab));

  /* Add the abovementioned 1st slab to the cache of caches */
  cache_add_slab(& fake_cache_of_caches,
		 vaddr_first_slab_of_caches,
		 slab_of_caches);

  /* Now we allocate a cache structure, which will be the real cache
     of caches, ie a cache structure allocated INSIDE the cache of
     caches, not inside the stack */
  real_cache_of_caches
    = (struct kslab_cache*) kvmm_cache_alloc(& fake_cache_of_caches,
						     0);
  /* We initialize it */
  memmove(real_cache_of_caches, & fake_cache_of_caches,
	 sizeof(struct kslab_cache));
  /* We need to update the slab's 'cache' field */
  slab_of_caches->cache = real_cache_of_caches;
  
  /* Add the cache to the list of slab caches */
  list_add_tail(kslab_cache_list, real_cache_of_caches);

  return real_cache_of_caches;
}

/**
 * Helper function to create the initial cache of ranges, with a very
 * first slab in it, so that new kvmm_range structures can be simply
 * allocated.
 * @return the cache of kvmm_range
 */
static struct kslab_cache *
create_cache_of_ranges(__u32 vaddr_first_slab_of_ranges,
		       __u32  sizeof_struct_range,
		       int nb_pages)
{
  /* The cache structure for the cache of kvmm_range */
  struct kslab_cache *cache_of_ranges;

  /* The kslab structure for the very first slab of ranges */
  struct kslab *slab_of_ranges;

  cache_of_ranges = (struct kslab_cache*)
    kvmm_cache_alloc(cache_of_struct_kslab_cache,
			 0);
  if (! cache_of_ranges)
    return NULL;

  /* Init the cache structure for the cache of ranges with min objects
     per slab = 2 !!! */
  if (init_cache(cache_of_ranges,
		       "struct kvmm_range",
		       sizeof_struct_range,
		       nb_pages, 2, KSLAB_CREATE_MAP | ON_SLAB))
    /* Something wrong with the parameters */
    return NULL;

  /* Add the cache to the list of slab caches */
  list_add_tail(kslab_cache_list, cache_of_ranges);





  /* Add the pages for the 1st slab of ranges */
  slab_of_ranges = (struct kslab*)(vaddr_first_slab_of_ranges
				       + nb_pages*PAGE_SIZE
				       - sizeof(struct kslab));


  cache_add_slab(cache_of_ranges,
		 vaddr_first_slab_of_ranges,
		 slab_of_ranges);

  return cache_of_ranges;
}

struct kslab_cache *
kvmm_cache_setup_prepare(__u32  kernel_core_base,
				       __u32  kernel_core_top,
				       __u32   sizeof_struct_range,
                                       struct kslab **first_struct_slab_of_caches,
				       __u32  *first_slab_of_caches_base,
				       __u32  *first_slab_of_caches_nb_pages,
                                        struct kslab **first_struct_slab_of_ranges,
				       __u32  *first_slab_of_ranges_base,
				       __u32  *first_slab_of_ranges_nb_pages)
{
  int i;
  int      retval;
  __u32  vaddr;

  /* The cache of ranges we are about to allocate */
  struct kslab_cache *cache_of_ranges;

  /* In the begining, there isn't any cache */
  kslab_cache_list = NULL;
  cache_of_struct_kslab = NULL;
  cache_of_struct_kslab_cache = NULL;

  /*
   * Create the cache of caches, initialised with 1 allocated slab
   */

  /* Allocate the pages needed for the 1st slab of caches, and map them
     in kernel space, right after the kernel */
  *first_slab_of_caches_base = PAGE_ALIGN_SUP(kernel_core_top);



  for (i = 0, vaddr = *first_slab_of_caches_base ;
       i < NB_PAGES_IN_SLAB_OF_CACHES ;
       i++, vaddr += PAGE_SIZE)
    {
      __u32  ppage_paddr;

      ppage_paddr
	= physmem_ref_physpage_new(false);
      if(ppage_paddr == (__u32 )NULL)
     debug();
        
      if(paging_unmap(vaddr))
           debug();
      retval = paging_map(vaddr, ppage_paddr,false);
      if(retval != 0 )
     debug();
   
      
    }

  /* Create the cache of caches */
  *first_slab_of_caches_nb_pages = NB_PAGES_IN_SLAB_OF_CACHES;
  cache_of_struct_kslab_cache
    = create_cache_of_caches(*first_slab_of_caches_base,
			     NB_PAGES_IN_SLAB_OF_CACHES);
  if(cache_of_struct_kslab_cache == NULL)
    debug();
     
/* Retrieve the slab that should have been allocated */
  *first_struct_slab_of_caches
    = list_get_head(cache_of_struct_kslab_cache->slab_list);


  /*
   * Create the cache of slabs, without any allocated slab yet
   */
  cache_of_struct_kslab
    = kvmm_cache_create("off-slab slab structures",
			    sizeof(struct kslab ),
			    1,
			    0,
			    KSLAB_CREATE_MAP);
  if(cache_of_struct_kslab == NULL)
     debug();

 


  
  /*
   * Create the cache of ranges, initialised with 1 allocated slab
   */
  /* Allocate the 1st slab */


  *first_slab_of_ranges_base = vaddr;

  for (i = 0;
       i < NB_PAGES_IN_SLAB_OF_RANGES ;
       i++, vaddr += PAGE_SIZE)
    {
      __u32  ppage_paddr;

      ppage_paddr
	= physmem_ref_physpage_new(false);
      if(ppage_paddr == (__u32 )NULL)
      debug();

      
      if (paging_unmap(vaddr))
          debug();
      retval = paging_map(vaddr, ppage_paddr ,false );
      if(retval != 0)
       debug();

    }


  /* Create the cache of ranges */
  *first_slab_of_ranges_nb_pages = NB_PAGES_IN_SLAB_OF_RANGES;
  cache_of_ranges = create_cache_of_ranges(*first_slab_of_ranges_base,
					   sizeof_struct_range,
					   NB_PAGES_IN_SLAB_OF_RANGES);
  if(cache_of_ranges == NULL)
   debug();
       /* Retrieve the slab that should have been allocated */
  *first_struct_slab_of_ranges
    = list_get_head(cache_of_ranges->slab_list);     

  return cache_of_ranges;
}

int kvmm_cache_subsystem_setup_commit(struct kslab *first_struct_slab_of_caches,
				      struct kvmm_range *first_range_of_caches,
				      struct kslab *first_struct_slab_of_ranges,
				      struct kvmm_range *first_range_of_ranges)
{
  first_struct_slab_of_caches->range = first_range_of_caches;
  first_struct_slab_of_ranges->range = first_range_of_ranges;
  return 0;
}

