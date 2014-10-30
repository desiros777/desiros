

#include <debug.h>
#include <kmalloc.h>
#include <physmem.h>
#include <kvmm_slab.h>
#include <list.h>
#include <mm.h>
#include <klibc.h>
#include <types.h>

#include <zero.h>


/**
 * A mapped page for a shared mapping of /dev/zero
 */
struct zero_mapped_page
{
  __u32 page_id;
  __u32   ppage_paddr;

  struct zero_mapped_page *prev, *next;
};
/** The Slab cache of shared mapped pages */
struct kslab_cache * cache_of_zero_mapped_pages;


/**
 * A mapped /dev/zero resource
 */
struct zero_mapped_resource
{
  int ref_cnt;

  /**
   * For shared mappings: the list of shared pages mapped inside one
   * or multiple arenas
   */
  struct zero_mapped_page *list_mapped_pages;

  struct uvmm_mapped_resource mr;
};


/** Helper function to insert the given physical page in the list of
    physical pages used for shared anonymous mappings */
static int insert_anonymous_physpage(struct zero_mapped_resource *mr,
					   __u32 ppage_paddr,
					   __u32 page_id);


/** Helper function to insert the given physical page in the list of
    physical pages used for shared anonymous mappings */
static __u32 lookup_anonymous_physpage(struct zero_mapped_resource *mr,
					     __u32 page_id);


int dev_zero_subsystem_setup()
{
  cache_of_zero_mapped_pages =
    kvmm_cache_create("shared anonymous mappings",
			  sizeof(struct zero_mapped_page),
			  1, 0,
			  KSLAB_CREATE_MAP | KSLAB_CREATE_ZERO);
  if (! cache_of_zero_mapped_pages)
    return -3;

 

  return 0;
}


/** Called after the virtual region has been inserted inside its
    address space */
static void zero_ref(struct uvmm_arena * arena)
{
  /* Retrieve the 'zero' structure associated with the mapped resource */
  struct zero_mapped_resource * zero_resource;
  zero_resource
    = (struct zero_mapped_resource*)
    uvmm_get_mapped_resource_of_arena(arena)->custom_data;
  
  /* Increment ref counter */
  zero_resource->ref_cnt ++;
}


/** Called when the virtual region is removed from its address
    space */
static void zero_unref(struct uvmm_arena * arena)
{
  /* Retrieve the 'zero' structure associated with the mapped resource */
  struct zero_mapped_resource * zero_resource;
  zero_resource
    = (struct zero_mapped_resource*)
    uvmm_get_mapped_resource_of_arena(arena)->custom_data;

  /* Decrement ref coutner */
  if(zero_resource->ref_cnt < 0) debug();
  zero_resource->ref_cnt --;

  /* Free the resource if it becomes unused */
  if (zero_resource->ref_cnt == 0)
    {
      /* Delete the list of anonymous shared mappings */
      struct zero_mapped_page *zmp;
      list_collapse(zero_resource->list_mapped_pages, zmp)
	{
	  /* Unreference the underlying physical page */
	  physmem_unref_physpage(zmp->ppage_paddr);
	  kfree((__u32)zmp);
	}

     kfree((__u32)zero_resource);
    }
}


/** MOST IMPORTANT callback ! Called when a thread page faults on the
    resource's mapping */
static int zero_no_page(struct uvmm_arena * arena,
			      __u32 uaddr,
			      bool write_access)
{

 

  int retval = 0;
  __u32 ppage_paddr;
  __u32 required_page_id;
  struct zero_mapped_resource * zero_resource;
  __u32 arena_prot, arena_flags;


  /* Retrieve the 'zero' structure associated with the mapped resource */
  zero_resource
    = (struct zero_mapped_resource*)
    uvmm_get_mapped_resource_of_arena(arena)->custom_data;

  /* Retrieve access rights/flags of the arena */
  arena_prot  = uvmm_get_prot_of_arena(arena);
  arena_flags = uvmm_get_flags_of_arena(arena);

  /* Identifies the page in the mapping that's being paged-in */
  required_page_id = PAGE_ALIGN_INF(uaddr)
    - uvmm_get_start_of_arena(arena)
    + uvmm_get_offset_in_resource(arena);

  /* For shared mappings, check if there is a page already mapping the
     required address */
  if (arena_flags & ARENA_MAP_SHARED)
    {
      ppage_paddr = lookup_anonymous_physpage(zero_resource, required_page_id);
      if (NULL != (void*)ppage_paddr)
	{
	  retval = paging_map(PAGE_ALIGN_INF(uaddr),ppage_paddr,
				  true);

	  return retval;
	}
    }

  /* For write accesses, directly maps a new page. For read accesses,
     simply map in the zero_page (and wait for COW to handle the next
     write accesses) */
  if (write_access)
    {


      /* Allocate a new page for the virtual address */
      ppage_paddr = physmem_ref_physpage_new(false);
      if (! ppage_paddr)
	return -3;

      retval = paging_map( PAGE_ALIGN_INF(uaddr),ppage_paddr ,
			      true);
      if (0 != retval)
	{
	  physmem_unref_physpage(ppage_paddr);
	  return retval;
	}
      
      memset((void*)PAGE_ALIGN_INF(uaddr), 0x0, PAGE_SIZE);

      /* For shared mappings, add the page in the list of shared
	 mapped pages */
      if (arena_flags & ARENA_MAP_SHARED)
	insert_anonymous_physpage(zero_resource, ppage_paddr,
				  required_page_id);

      physmem_unref_physpage(ppage_paddr);
    }
  else
    {
                
      /* Map-in the zero page in READ ONLY whatever the access_rights
	 or the type (shared/private) of the arena to activate COW */
      retval = paging_map(PAGE_ALIGN_INF(uaddr),zero_page,
			      true );

       

    }

  return retval;
}


/** The callbacks for a mapped /dev/zero resource */
static struct uvmm_arena_ops zero_ops = (struct uvmm_arena_ops)
{
  .ref     = zero_ref,
  .unref   = zero_unref,
  .no_page = zero_no_page,
  .unmap   = NULL
};


/** The callback that gets called when the resource gets mapped */
static __u32 zero_mmap(struct uvmm_arena *arena)
{
  return uvmm_set_ops_of_arena(arena, &zero_ops);
}


/** The function responsible for mapping the /dev/zero resource in
    user space */
int dev_zero_map(struct uvmm_as* dest_as,
			   __u32 *uaddr,
			   __u32 size,
			   __u32 access_rights,
			   __u32 flags)
{
  int retval;
  struct zero_mapped_resource * zero_resource;
  zero_resource
    = (struct zero_mapped_resource*) kmalloc(sizeof(*zero_resource), 0);
  if (! zero_resource)
    return -3;

      

  memset(zero_resource, 0x0, sizeof(*zero_resource));
  zero_resource->mr.allowed_access_rights 
    = P_READ | P_WRITE | P_USER;
  zero_resource->mr.flags         |= MAPPED_RESOURCE_ANONYMOUS;
  zero_resource->mr.custom_data    = zero_resource;
  zero_resource->mr.mmap           = zero_mmap;

 
   
  retval = uvmm_map(dest_as, uaddr, size,
			    access_rights, flags,
			    &zero_resource->mr, 0);
  if (0 != retval)
    {
      kfree((__u32)zero_resource);
      return retval;
    }

  return 0;
}


static int insert_anonymous_physpage(struct zero_mapped_resource *mr,
					   __u32 ppage_paddr,
					   __u32 page_id)
{
  struct zero_mapped_page * zmp
    = (struct zero_mapped_page*)kvmm_cache_alloc(cache_of_zero_mapped_pages,
						     0);
  if (! zmp)
    return -3;

  zmp->page_id     = page_id;
  zmp->ppage_paddr = ppage_paddr;

  list_add_head(mr->list_mapped_pages, zmp);
  physmem_ref_physpage_at(ppage_paddr);
  return 0;
}


static __u32 lookup_anonymous_physpage(struct zero_mapped_resource *mr,
					    __u32 page_id)
{
  struct zero_mapped_page * zmp;
  int nb_elts;

  list_foreach_forward(mr->list_mapped_pages, zmp, nb_elts)
    {
      if (zmp->page_id == page_id)
	return zmp->ppage_paddr;
    }
  
  return (__u32)NULL;
}
