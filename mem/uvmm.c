
#include <debug.h>
#include <uvmm.h>
#include <kvmm.h>
#include <mm.h>
#include <types.h>
#include <process.h>
#include <physmem.h>
#include <list.h>
#include <macros.h>
#include <kerrno.h>

struct uvmm_as
{
  /** The process that owns this address space */
  struct process     * process;

 

  /** The list of arenas in this address space */
  struct uvmm_arena * list_arena;

  /** Heap location */
  __u32  heap_start;
  __u32  heap_size; /**< Updated by uvmm_brk() */

  /* Memory usage statistics */
  __u32 phys_total; /* shared + private */
 
  struct vm_usage
  {
    __u32 overall;
    __u32 ro, rw, code /* all: non readable, read and read/write */;
  } vm_total, vm_shrd;


  /* Page fault counters */
  __u32 pgflt_page_in;
  __u32 pgflt_invalid;
};

/* arena or virtual region or zone */
struct uvmm_arena
{
  /** The address space owning this arena */
  struct uvmm_as *address_space;

  /** The location of the mapping in user space */
  __u32 start;
  __u32  size;

  /** What accesses are allowed (read, write, exec)*/
  __u32  access_rights;

  /** Flags of the arena. Allowed flags:
   *  - ARENA_MAP_SHARED
   */
  __u32  flags;

  /**
   * The callbacks for the arena called along map/unmapping of the
   * resource
   */
  struct uvmm_arena_ops *ops;

  /** Description of the resource being mapped, if any */
  struct uvmm_mapped_resource *mapped_resource;
  __u64 offset_in_resource;

  /** The arenas of an AS are linked together and are accessible by way
      of as->list_arena */
  struct uvmm_arena *prev_in_as, *next_in_as;

  /** The arenas mapping a given resource are linked together and are
      accessible by way of mapped_resource->list_arena */
  struct uvmm_arena *prev_in_mapped_resource, *next_in_mapped_resource;
};


/*
 * We use special slab caches to allocate AS and ARENA data structures
 */
static struct kslab_cache * cache_of_as;
static struct kslab_cache * cache_of_arena;


__u32 zero_page = 0 /* Initial value prior to allocation */;


static void
as_account_change_of_arena_prot(struct  uvmm_as * as,
				   bool is_shared,
				   __u32 size,
				   __u32 prev_access_rights,
				   __u32 new_access_rights);


int uvmm_subsystem_setup()
{
  __u32 vaddr_zero_page;

  /* Allocate a new kernel physical page mapped into kernel space and
     reset it with 0s */


  vaddr_zero_page = kvmm_alloc(1, KVMM_MAP);


  if (vaddr_zero_page == (__u32)NULL){
    debug();
    return -3;
    }
  memset((void*)vaddr_zero_page, 0x0, PAGE_SIZE);

  
  /* Keep a reference to the underlying pphysical page... */
  zero_page =  paging_virtual_to_physical( page_directory,vaddr_zero_page);
  if(NULL == (void*)zero_page){

               debug();
               return 0 ;

  }
           
  physmem_ref_physpage_at(zero_page);

  /* ... but it is not needed in kernel space anymore, so we can
     safely unmap it from kernel space */
  paging_unmap(vaddr_zero_page);

  /* Allocate the arena/AS caches */



  cache_of_as
    = kvmm_cache_create("Address space structures",
			    sizeof(struct uvmm_as),
			    1, 0,
			    KSLAB_CREATE_MAP
			    | KSLAB_CREATE_ZERO);
  if (! cache_of_as)
    {
      debug();
      physmem_unref_physpage(zero_page);
      return -3;
    }

  cache_of_arena
    = kvmm_cache_create("Arena structures",
			    sizeof(struct uvmm_arena),
			    1, 0,
			    KSLAB_CREATE_MAP
			    | KSLAB_CREATE_ZERO);
  if (! cache_of_arena)
    {
      debug();
      physmem_unref_physpage(zero_page);
      kvmm_cache_destroy(cache_of_as);
      return -3;
    }

/* For  paging_pd_add_pt() */
 cache_pt = kvmm_cache_create("Page tables",
			    sizeof(struct page_table),
			    1, 0,
			    KSLAB_CREATE_MAP
			    | KSLAB_CREATE_ZERO);

  return 0;
}


struct uvmm_as *uvmm_create_empty_as(struct process *owner)
{
    
  struct uvmm_as * as
    = (struct uvmm_as *) kvmm_cache_alloc(cache_of_as, 0);
  if (! as){
    return NULL;
    }

  as->process = owner;
  return as;
}

int uvmm_delete_as(struct uvmm_as * as)
{
  while(! list_is_empty_named(as->list_arena, prev_in_as, next_in_as))
    {
      struct uvmm_arena * arena;
      arena = list_get_head_named(as->list_arena, prev_in_as, next_in_as);

      /* Remove the arena from the lists */
      list_pop_head_named(as->list_arena, prev_in_as, next_in_as);
      list_delete_named(arena->mapped_resource->list_arena, arena,
			prev_in_mapped_resource,
			next_in_mapped_resource);

      /* Signal to the underlying arena mapper that the mapping is
	 suppressed */
      if (arena->ops)
	{
	  if (arena->ops->unmap)
	    arena->ops->unmap(arena, arena->start, arena->size);
	  if (arena->ops->unref)
	    arena->ops->unref(arena);
	}

      kvmm_cache_free((__u32)arena);
    }
  
  

  /* Now unallocate main address space construct */
  kvmm_cache_free((__u32)as);

  return 0;
}


static struct uvmm_arena *
find_enclosing_or_next_arena(struct uvmm_as* as,
			  __u32 uaddr)
{
  struct uvmm_arena *arena;
  int nb_arena;

  if (uaddr > PAGING_TOP_USER_ADDRESS) 
    return NULL;
 
  list_foreach_named(as->list_arena, arena, nb_arena, prev_in_as, next_in_as)
    {
      /* Equivalent to "if (uaddr < arena->start + arena->size)" but more
	 robust (resilient to integer overflows) */
      if (uaddr <= arena->start + (arena->size - 1))
	return arena;
    }

  return NULL;
}



static struct uvmm_arena *
find_first_intersecting_arena(struct uvmm_as * as,
			   __u32 start_uaddr, __u32 size)
{
  struct uvmm_arena *arena;
  arena = find_enclosing_or_next_arena(as, start_uaddr);
  if (! arena)
    return NULL;


  return arena;
}



int uvmm_init_heap(struct uvmm_as * as, __u32 heap_start)
{
  if(! as->heap_start)
    debug("heap_start undefined\n");

  as->heap_start = heap_start;
  as->heap_size  = 0;
  return 0 ;
}


static __u32
find_first_free_interval(struct uvmm_as * as,
			 __u32 hint_uaddr, __u32 size)
{
  struct uvmm_arena * initial_arena, * arena;

  if (hint_uaddr < USER_OFFSET)
    hint_uaddr = USER_OFFSET;

  if (hint_uaddr > PAGING_TOP_USER_ADDRESS - size + 1)
    return (__u32)NULL;

  initial_arena = arena = find_enclosing_or_next_arena(as, hint_uaddr);
  if (! arena)
    /* Great, there is nothing after ! */
    return hint_uaddr;

  /* Scan the remaining arenas in the list */
  do
    {
      /* Is there enough space /before/ that arena ? */
      if (hint_uaddr + size <= arena->start)
	/* Great ! */
	return hint_uaddr;

      /* Is there any arena /after/ this one, or do we have to wrap back
	 at the begining of the user space ? */
      if (arena->next_in_as->start >= hint_uaddr)
	/* Ok, the next arena is really after us */
	hint_uaddr = arena->start + arena->size;
      else
	{
	  /* No: wrapping up */

	  /* Is there any space before the end of user space ? */
	  if (hint_uaddr <= PAGING_TOP_USER_ADDRESS - size)
	    return hint_uaddr;

	  hint_uaddr = USER_OFFSET;
	}

      /* Prepare to look after this arena */
      arena = arena->next_in_as;
    }
  while (arena != initial_arena);

  /* Reached the end of the list and did not find anything ?... Look
     at the space after the last arena */

  return (__u32)NULL;
}


int uvmm_unmap(struct uvmm_as * as,
		   __u32 uaddr, __u32 size)
{
  struct uvmm_arena *arena, *preallocated_arena;
  bool used_preallocated_arena;

  if (! IS_PAGE_ALIGNED(uaddr))
    return -1;
  if (size <= 0)
    return -1;
  size = PAGE_ALIGN_SUP(size);

  /* Make sure the uaddr is valid */
  if (uaddr < USER_OFFSET)
    return -1;
  if (uaddr > PAGING_TOP_USER_ADDRESS - size)
    return -1;

  /* In some cases, the unmapping might imply a arena to be split into
     2. Actually, allocating a new arena can be a blocking operation, but
     actually we can block now, it won't do no harm. But we must be
     careful not to block later, while altering the arena lists: that's
     why we pre-allocate now. */
  used_preallocated_arena = false;
  preallocated_arena
    = (struct uvmm_arena *)kvmm_cache_alloc(cache_of_arena, 0);
  if (! preallocated_arena)
    return -3;

  /* Find any arena intersecting with the given interval */
  arena = find_first_intersecting_arena(as, uaddr, size);

  /* Unmap (part of) the arena covered by [uaddr .. uaddr+size[ */
  while (NULL != arena)
    {
      /* Went past the end of the *circular* list => back at the
	 beginning ? */
      if (arena->start + arena->size <= uaddr)
	/* Yes, stop now */
	break;

      /* Went beyond the region to unmap ? */
      if (uaddr + size <= arena->start)
	/* Yes, stop now */
	break;



      /* arena totally unmapped ? */
      if ((arena->start >= uaddr)
	  && (arena->start + arena->size <= uaddr + size))
	{
	  struct uvmm_arena *next_arena;

           
	  /* Yes: signal we remove it completely */
	  if (arena->ops && arena->ops->unmap)
	    arena->ops->unmap(arena, arena->start, arena->size);

	  /* Remove it from the AS list now */
	  next_arena = arena->next_in_as;
	  if (next_arena == arena) /* singleton ? */
	    next_arena = NULL;
	  list_delete_named(as->list_arena, arena, prev_in_as, next_in_as);

	  /* Remove from the list of arenas mapping the resource */
	  list_delete_named(arena->mapped_resource->list_arena, arena,
			    prev_in_mapped_resource,
			    next_in_mapped_resource);

	  if (arena->ops && arena->ops->unref)
	    arena->ops->unref(arena);
	  
	  as_account_change_of_arena_prot(as, arena->flags & ARENA_MAP_SHARED,
					     arena->size, arena->access_rights, 0);
	  kvmm_free((__u32)arena);
      
	  /* Prepare next iteration */
	  arena = next_arena;
	  continue;
	}

      /* unmapped region lies completely INSIDE the the arena */
      else if ( (arena->start < uaddr)
		&& (arena->start + arena->size > uaddr + size) )
	{
	  /* arena has to be split into 2 */

	  /* Use the preallocated arena and copy the arena into it */
	  used_preallocated_arena = true;
	  memcpy(preallocated_arena, arena, sizeof(*arena));

	  /* Adjust the start/size of both arenas */
	  preallocated_arena->start = uaddr + size;
	  preallocated_arena->size  = arena->start + arena->size - (uaddr + size);
	  preallocated_arena->offset_in_resource += uaddr + size - arena->start;
	  arena->size                             = uaddr - arena->start;

	  /* Insert the new arena into the list */
	  list_insert_after_named(as->list_arena, arena, preallocated_arena,
				  prev_in_as, next_in_as);
	  list_add_tail_named(arena->mapped_resource->list_arena, preallocated_arena,
			      prev_in_mapped_resource,
			      next_in_mapped_resource);

	  /* Signal the changes to the underlying resource */
	  if (arena->ops && arena->ops->unmap)
	    arena->ops->unmap(arena, uaddr, size);
	  if (preallocated_arena->ops && preallocated_arena->ops->ref)
	    preallocated_arena->ops->ref(preallocated_arena);

	  /* Account for change in arenas */
	  as_account_change_of_arena_prot(as, arena->flags & ARENA_MAP_SHARED,
					     size, arena->access_rights, 0);

	  /* No need to go further */
	  break;
	}

      /* Unmapped region only affects the START address of the arena */
      else if (uaddr <= arena->start)
	{

	  __u32 translation = uaddr + size - arena->start;

	  /* Shift the arena */
	  arena->size               -= translation;
	  arena->offset_in_resource += translation;
	  arena->start              += translation;
	  
	  /* Signal unmapping */
	  if (arena->ops && arena->ops->unmap)
	    arena->ops->unmap(arena, uaddr + size,
			   translation);
	  
	  /* Account for change in arenas */
	  as_account_change_of_arena_prot(as, arena->flags & ARENA_MAP_SHARED,
					     translation,
					     arena->access_rights, 0);

	  /* No need to go further, we reached the last arena that
	     overlaps the unmapped region */
	  break;
	}

      /* Unmapped region only affects the ENDING address of the arena */
      else if (uaddr + size >= arena->start + arena->size)
	{
          
	  __u32 unmapped_size = arena->start + arena->size - uaddr;

	  /* Resize arena */
	  arena->size = uaddr - arena->start;
	  
	  /* Signal unmapping */
	  if (arena->ops && arena->ops->unmap)
	    arena->ops->unmap(arena, uaddr, unmapped_size);

	  /* Account for change in arenas */
	  as_account_change_of_arena_prot(as, arena->flags & ARENA_MAP_SHARED,
					     unmapped_size,
					     arena->access_rights, 0);
	  
	  arena = arena->next_in_as;
	  continue;
	}

      debug("uaddr=%x sz=%x arena_start=%x, arena_sz=%x",
			      uaddr, size, arena->start, arena->size);
    }


    

    __u32 sz_unmapped = paging_unmap_interval(uaddr, size);

    if( 0 >= sz_unmapped) debug();
    as->phys_total -= sz_unmapped;
      

 

  if (! used_preallocated_arena)
    kvmm_free((__u32)preallocated_arena);

  return 0 ;
}

#define INTERNAL_MAP_CALLED_FROM_MREMAP (1 << 8)


int uvmm_resize(struct uvmm_as * as,
		    __u32 old_uaddr, __u32 old_size,
		    __u32 *new_uaddr, __u32 new_size,
		    __u32 flags)
{
  __u64 new_offset_in_resource;
  bool must_move_arena = false;
  struct uvmm_arena *arena, *prev_arena, *next_arena;

  /* Make sure the new uaddr is valid */
  if (! PAGING_IS_USER_AREA(*new_uaddr, new_size) )
    return -EPERM ;

  old_uaddr = PAGE_ALIGN_INF(old_uaddr);
  old_size  = PAGE_ALIGN_SUP(old_size);
  if (! IS_PAGE_ALIGNED(*new_uaddr))
    return -EPERM ;
  if (new_size <= 0)
    return -EPERM ;
  new_size = PAGE_ALIGN_SUP(new_size);
  
  /* Lookup a arena overlapping the address range */
  arena = find_first_intersecting_arena(as, old_uaddr, old_size);
  if (! arena)
    return -EPERM ;
  
  /* Make sure there is exactly ONE arena overlapping the area */
  if ( (arena->start > old_uaddr)
       || (arena->start + arena->size < old_uaddr + old_size) )
    return -EPERM ;

  /* Retrieve the prev/next arena if they exist (the arena are on circular
     list) */
  prev_arena = arena->prev_in_as;
  if (prev_arena->start >= arena->start)
    prev_arena = NULL;
  next_arena = arena->prev_in_as;
  if (next_arena->start <= arena->start)
    next_arena = NULL;

  /*
   * Compute new offset inside the mapped resource, if any
   */

  /* Don't allow to resize if the uaddr goes beyond the 'offset 0' of
     the resource */
  if ( (*new_uaddr < arena->start)
       && (arena->start - *new_uaddr > arena->offset_in_resource) )
    return -EPERM ;
  
  /* Compute new offset in the resource (overflow-safe) */
  if (arena->start > *new_uaddr)
    new_offset_in_resource
      = arena->offset_in_resource
      - (arena->start - *new_uaddr);
  else
    new_offset_in_resource
      = arena->offset_in_resource
      + (*new_uaddr - arena->start);

  /* If other arenas would be affected by this resizing, then the arena must
     be moved */
  if (prev_arena && (prev_arena->start + prev_arena->size > *new_uaddr))
    must_move_arena |= true;
  if (next_arena && (next_arena->start < *new_uaddr + new_size))
    must_move_arena |= true;

  /* If VR would be out-of-user-space, it must be moved */
  if (! PAGING_IS_USER_AREA(*new_uaddr, new_size) )
    must_move_arena |= true;

  /* The arena must be moved but the user forbids it */
  if ( must_move_arena && !(flags & ARENA_REMAP_MAYMOVE) )
    return -EPERM ;

  /* If the arena must be moved, we simply map the resource elsewhere and
     unmap the current arena */
  if (must_move_arena)
    {
      __u32 uaddr, result_uaddr;
      int retval;

      result_uaddr = *new_uaddr;
      retval = uvmm_map(as, & result_uaddr, new_size,
				arena->access_rights,
				arena->flags | INTERNAL_MAP_CALLED_FROM_MREMAP,
				arena->mapped_resource,
				new_offset_in_resource);
      if (OK != retval)
	return retval;

      /* Remap the physical pages at their new address */
      for (uaddr = arena->start ;
	   uaddr < arena->start + arena->size ;
	   uaddr += PAGE_SIZE)
	{
	  __u32 paddr;
	  __u32 prot;
	  __u32 vaddr;
	  
	  if (uaddr < *new_uaddr)
	    continue;
	  if (uaddr > *new_uaddr + new_size)
	    continue;

	  /* Compute destination virtual address (should be
	     overflow-safe) */
	  if (arena->start >= *new_uaddr)
	    vaddr = result_uaddr
	      + (uaddr - arena->start)
	      + (arena->start - *new_uaddr);
	  else
	    vaddr = result_uaddr
	      + (uaddr - arena->start)
	      - (*new_uaddr - arena->start);

	  paddr = paging_virtual_to_physical(page_directory,uaddr);
	  if (! paddr)
	    /* No physical page mapped at this address yet */
	    continue;

	  /* Remap it at its destination address */
	  retval = paging_map(vaddr, paddr,true);
	  if (OK != retval)
	    {
	      uvmm_unmap(as, result_uaddr, new_size);
	      return retval;
	    }
	}

      retval = uvmm_unmap(as, arena->start, arena->size);
      if (OK != retval)
	{
	  uvmm_unmap(as, result_uaddr, new_size);
	  return retval;
	}

      *new_uaddr = result_uaddr;
      return retval;
    }

  /* Otherwise we simply resize the arena, taking care of unmapping
     what's been unmapped  */

  if (*new_uaddr + new_size < arena->start + arena->size)
    uvmm_unmap(as, *new_uaddr + new_size,
		       arena->start + arena->size - (*new_uaddr + new_size));
  else
    {
      as_account_change_of_arena_prot(as, arena->flags & ARENA_MAP_SHARED,
					 *new_uaddr + new_size
					   - (arena->start + arena->size),
					 0, arena->access_rights);
      arena->size += *new_uaddr + new_size - (arena->start + arena->size);
    }
  
  if (*new_uaddr > arena->start)
    uvmm_unmap(as, arena->start, *new_uaddr - arena->start);
  else
    {
      as_account_change_of_arena_prot(as, arena->flags & ARENA_MAP_SHARED,
					 arena->start - *new_uaddr,
					 0, arena->access_rights);
      arena->size  += arena->start - *new_uaddr;
      arena->start  = *new_uaddr;
      arena->offset_in_resource = new_offset_in_resource; 
    }


  return OK;
}


int uvmm_map(struct uvmm_as * as,
		 __u32 * /*in/out*/uaddr, __u32 size,
		 __u32 access_rights,
		 __u32 flags,
		 struct uvmm_mapped_resource * resource,
		 __u64 offset_in_resource)
{
  __label__ return_mmap;
  __u32 hint_uaddr;
  struct uvmm_arena *prev_arena, *next_arena, *arena, *preallocated_arena;
  bool merge_with_preceding, merge_with_next, used_preallocated_arena;
  bool internal_map_called_from_mremap
    = (flags & INTERNAL_MAP_CALLED_FROM_MREMAP);

  int retval     = 0;
  used_preallocated_arena = false;
  hint_uaddr = *uaddr;

  /* Default mapping address is NULL */
  *uaddr = (__u32)NULL;

  if (! resource)
    return -1;
  if (! resource->mmap)
    return -6;

  if (!IS_PAGE_ALIGNED(hint_uaddr))
    return -1;

  if (size <= 0)
    return -1;
  size = PAGE_ALIGN_SUP(size);

  if (flags & ARENA_MAP_SHARED)
    {
      /* Make sure the mapped resource allows the required protection flags */
      if ( ( (access_rights & P_READ)
	     && !(resource->allowed_access_rights & P_READ) )
	   || ( (access_rights & P_WRITE)
		&& !(resource->allowed_access_rights & P_WRITE) )
	   || ( (access_rights & P_USER)
		&& !(resource->allowed_access_rights & P_USER)) )
	return -6;
    }

  /* Sanity checks over the offset_in_resource parameter */
  if ( !internal_map_called_from_mremap
       && ( resource->flags & MAPPED_RESOURCE_ANONYMOUS ) )
    /* Initial offset ignored for anonymous mappings */
    {
      /* Nothing to check */
    }

  /* Make sure that the offset in resource won't overflow */
  else if (offset_in_resource + size <= offset_in_resource)
    return -1;

  /* Filter out unsupported flags */
  access_rights &= (P_READ
		    | P_WRITE
		    | P_USER );
  flags &= (ARENA_MAP_SHARED
	    | ARENA_MAP_FIXED);

  /* Pre-allocate a new arena. Because once we found a valid slot inside
     the arena list, we don't want the list to be altered by another
     process */

  preallocated_arena
    = (struct uvmm_arena *)kvmm_cache_alloc(cache_of_arena, 0);
  if (! preallocated_arena)
    return -3;

      
  /* Compute the user address of the new mapping */
  if (flags & ARENA_MAP_FIXED)
    {
      /*
       * The address is imposed
       */

      /* Make sure the hint_uaddr hint is valid */
      if (hint_uaddr < USER_OFFSET)
	{ debug(); retval = -1; goto return_mmap; }
      if (hint_uaddr > PAGING_TOP_USER_ADDRESS - size)
	{ debug(); retval = -1; goto return_mmap; }

      /* Unmap any overlapped arena */
      retval = uvmm_unmap(as, hint_uaddr, size);
      if (0 != retval)
	{ goto return_mmap; }
    }
  else
    {
      /*
       * A free range has to be determined
       */

      /* Find a suitable free arena */
      hint_uaddr = find_first_free_interval(as, hint_uaddr, size);
      if (! hint_uaddr)
	{ debug(); retval = -3; goto return_mmap; }
    }




  /* For anonymous resource mappings, set the initial
     offset_in_resource to the initial virtual start address in user
     space */
  if ( !internal_map_called_from_mremap
       && (resource->flags & MAPPED_RESOURCE_ANONYMOUS ) )
    offset_in_resource = hint_uaddr;

  /* Lookup next and previous arena, if any. This will allow us to merge
     the regions, when possible */
  next_arena = find_enclosing_or_next_arena(as, hint_uaddr);
  if (next_arena)
    {
      /* Find previous arena, if any */
      prev_arena = next_arena->prev_in_as;
      /* The list is curcular: it may happen that we looped over the
	 tail of the list (ie the list is a singleton) */
      if (prev_arena->start > hint_uaddr)
	prev_arena = NULL; /* No preceding arena */
    }
  else
    {
      /* Otherwise we went beyond the last arena */
      prev_arena = list_get_tail_named(as->list_arena, prev_in_as, next_in_as);
    }

  /* Merge with preceding arena ? */
  merge_with_preceding
    = ( (NULL != prev_arena)
	&& (prev_arena->mapped_resource == resource)
	&& (prev_arena->offset_in_resource + prev_arena->size == offset_in_resource)
	&& (prev_arena->start + prev_arena->size == hint_uaddr)
	&& (prev_arena->flags == flags)
	&& (prev_arena->access_rights == access_rights) );

  /* Merge with next arena ? */
  merge_with_next
    = ( (NULL != next_arena)
	&& (next_arena->mapped_resource == resource)
	&& (offset_in_resource + size == next_arena->offset_in_resource)
	&& (hint_uaddr + size == next_arena->start)
	&& (next_arena->flags == flags)
	&& (next_arena->access_rights == access_rights) );

  if (merge_with_preceding && merge_with_next)
    {
      /* Widen the prev_arena arena to encompass both the new arena and the next_arena */
      arena = prev_arena;
      arena->size += size + next_arena->size;
      
      /* Remove the next_arena arena */
      list_delete_named(as->list_arena, next_arena, prev_in_as, next_in_as);
      list_delete_named(next_arena->mapped_resource->list_arena, next_arena,
			prev_in_mapped_resource, next_in_mapped_resource);

      if (next_arena->ops && next_arena->ops->unref)
	next_arena->ops->unref(next_arena);
        

      kvmm_free((__u32) next_arena);
    }
  else if (merge_with_preceding)
    {
      /* Widen the prev_arena arena to encompass the new arena */
      arena = prev_arena;
      arena->size += size;
    }
  else if (merge_with_next)
    {
      /* Widen the next_arena arena to encompass the new arena */
      arena = next_arena;
      arena->start -= size;
      arena->size  += size;
    }
  else
    {
      /* Allocate a brand new arena and insert it into the list */

      arena = preallocated_arena;
      used_preallocated_arena = true;

      arena->start              = hint_uaddr;
      arena->size               = size;
      arena->access_rights      = access_rights;
      arena->flags              = flags;
      arena->mapped_resource    = resource;
      arena->offset_in_resource = offset_in_resource;

      /* Insert arena in address space */
      arena->address_space      = as;
      if (prev_arena)
	list_insert_after_named(as->list_arena, prev_arena, arena,
				prev_in_as, next_in_as);
      else
	list_add_head_named(as->list_arena, arena, prev_in_as, next_in_as);
      list_add_tail_named(arena->mapped_resource->list_arena, arena,
			  prev_in_mapped_resource,
			  next_in_mapped_resource);
      
       
      /* Signal the resource we are mapping it */
      if (resource && resource->mmap)
	{
	  retval = resource->mmap(arena);

	  if (0 != retval){
              retval = uvmm_unmap(as, arena->start, arena->size);
	      goto return_mmap;
	    }

	  /* The page_in method is MANDATORY for mapped resources */
	  if(!arena->ops && !arena->ops->no_page)
                debug();
	}

      if (arena->ops && arena->ops->ref)
	arena->ops->ref(arena);
    }

  /* Ok, fine, we got it right ! Return the address to the caller */
  *uaddr = hint_uaddr;



 as_account_change_of_arena_prot(as, arena->flags & ARENA_MAP_SHARED,
				     size, 0, arena->access_rights);


  retval = 0;

 return_mmap:
  if (! used_preallocated_arena)
    kvmm_free((__u32)preallocated_arena);
    
    
  return retval;
}


__u32 uvmm_brk(struct uvmm_as * as,
		 __u32 new_top_uaddr)
{
  __u32 new_start;
  __u32  new_size;
  if(!as->heap_start) debug();

  if (! new_top_uaddr)
    return as->heap_start + as->heap_size;

  if (new_top_uaddr == as->heap_start + as->heap_size)
    return as->heap_start + as->heap_size;
 
  if (new_top_uaddr < as->heap_start)
    return (__u32)NULL;

  new_top_uaddr = PAGE_ALIGN_SUP(new_top_uaddr);
  new_start = as->heap_start;
  new_size  = new_top_uaddr - as->heap_start;

  /* First call to brk: we must map /dev/zero */
  if (! as->heap_size)
    {
      if (OK != dev_zero_map(as, & as->heap_start,
				     new_size,
				     P_READ
				     | P_WRITE,
				     0 /* private non-fixed */))
	return (__u32)NULL;

      as->heap_size = new_size;
      return as->heap_start + as->heap_size;
    }

  /* Otherwise we just have to unmap or resize the region */
  if (new_size <= 0)
    {
      if (OK != uvmm_unmap(as,
				       as->heap_start, as->heap_size))
	return (__u32)NULL;
    }
  else
    {
      if (OK != uvmm_resize(as,as->heap_start, as->heap_size,
					& new_start, new_size,
					0))
	return (__u32)NULL;
    }

  if(new_start == as->heap_start) debug();
  as->heap_size = new_size;
  return new_top_uaddr;
}


static void
as_account_change_of_arena_prot(struct  uvmm_as * as,
				   bool is_shared,
				   __u32 size,
				   __u32 prev_access_rights,
				   __u32 new_access_rights)
{
  if (prev_access_rights == new_access_rights)
    return;

#define _UPDATE_VMSTAT(field,is_increment) \
  ({ if (is_increment > 0) \
       as->field += size; \
     else \
       { if( size >= as->field) debug(); as->field -= size; } })

#define UPDATE_VMSTAT(field,is_increment) \
  ({ if (is_shared) _UPDATE_VMSTAT(vm_shrd.field, is_increment); \
     _UPDATE_VMSTAT(vm_total.field, is_increment); \
     if(as->vm_shrd.field >= as->vm_total.field) debug(); }) 

  if ( (new_access_rights & P_WRITE)
       && !(prev_access_rights & P_WRITE))
    {
      UPDATE_VMSTAT(rw, +1);
      if (prev_access_rights & P_READ)
	UPDATE_VMSTAT(ro, -1);
    }
  else if ( !(new_access_rights & P_WRITE)
	    && (prev_access_rights & P_WRITE))
    {
      if (new_access_rights & P_READ)
	UPDATE_VMSTAT(ro, +1);
      UPDATE_VMSTAT(rw, -1);
    }
  else if (new_access_rights & P_READ)
    UPDATE_VMSTAT(ro, +1);
  else if (!(new_access_rights & P_READ))
    UPDATE_VMSTAT(ro, -1);

  if ( (new_access_rights & P_USER)
       && !(prev_access_rights & P_USER))
    {
      UPDATE_VMSTAT(code, +1);
    }
  else if ( !(new_access_rights & P_USER)
	    && (prev_access_rights & P_USER))
    {
      UPDATE_VMSTAT(code, -1);
    }

  if (new_access_rights && !prev_access_rights)
    UPDATE_VMSTAT(overall, +1);
  else if (!new_access_rights && prev_access_rights)
    UPDATE_VMSTAT(overall, -1);

}

struct uvmm_mapped_resource *uvmm_get_mapped_resource_of_arena(struct uvmm_arena * arena)
{
  return arena->mapped_resource;
}

__u32 uvmm_get_start_of_arena(struct uvmm_arena * arena)
{
  return arena->start;
}

__u64 uvmm_get_offset_in_resource(struct uvmm_arena * arena)
{
  return arena->offset_in_resource;
}

int uvmm_set_ops_of_arena(struct uvmm_arena * arena,
			   struct uvmm_arena_ops * ops)
{
  /* Don't allow to overwrite any preceding arena ops */
  if(NULL != arena->ops) debug();

  arena->ops = ops;
  return 0;
}

int uvmm_get_prot_of_arena(struct uvmm_arena * arena)
{
  return arena->access_rights;
}

int uvmm_get_flags_of_arena(struct uvmm_arena * arena)
{
  return arena->flags;
}

int uvmm_lazy_loading(__u32 uaddr,
					      bool write_access,
					      bool user_access)
{


  struct process     *process = current ;
  struct uvmm_as *as;
  struct uvmm_arena *arena;

  if (! process)
   {
    debug();
    return -7;
    }

 

  as = process_get_address_space(process);
  if (! as)
     {
    debug();
    return -7;
    }


  arena = find_first_intersecting_arena(as, uaddr, 1);
  if (! arena)
     {
    debug();
    return -7;
    }


  /* Ask the underlying resource to resolve the page fault */
  if ( 0 != arena->ops->no_page(arena, uaddr, write_access))
    {
      debug();
      as->pgflt_invalid ++;
      return -7;
    }

  as->phys_total += PAGE_SIZE;
  as->pgflt_page_in ++;

   	
            

  return 0;
}



