#ifndef _UVMM_H_
#define _UVMM_H_

#include <types.h>
#include <process.h>



struct uvmm_as;

struct uvmm_arena;


/**
 * Physical address of THE page (full of 0s) used for anonymous
 * mappings. Anybody can map it provided it is ALWAYS in read-only
 * mode
 */
extern __u32 zero_page;


/** arena flag: region can be shared between a process and its
    children */
#define ARENA_MAP_SHARED (1 << 0)

/** uvmm_map() flag: the address given as parameter to
    uvmm_map() is not only a hint, it is where the arena is
    expected to be mapped */
#define ARENA_MAP_FIXED  (1 << 31)

/** Inidicate that this resource is not backed by any physical
    storage. This means that the "offset_in_resource" field of the
    arenas will be computed by uvmm_map() */
#define MAPPED_RESOURCE_ANONYMOUS (1 << 0)

/**
 * Flag for uvmm_resize() to indicate that the arena being
 * resized can be moved elsewhere if there is not enough room to
 * resize it in-place
 */
#define ARENA_REMAP_MAYMOVE (1 << 30)

struct uvmm_arena_ops
{
  /**
   * Called after the arena has been inserted
   * inside its address space.
   * @note Optional
   */
  void (*ref)(struct uvmm_arena * arena);

  /**
   * Called when the arena is removed from its
   * address space
   * @note Optional
   */
  void (*unref)(struct uvmm_arena * arena);

  /**
   * Called when part or all a arena is unmapped
   * @note Optional
   */
  void (*unmap)(struct uvmm_arena * arena,
		__u32 uaddr, __u32 size);

  /**
   * Called by the page fault handler to map data at the given virtual
   * address. In the Linux kernel, this callback is named "nopage".
   *
   * @note MANDATORY
   */
  int (*no_page)(struct uvmm_arena * arena,
		       __u32 uaddr,
		       bool write_access);
};

/**
 * The definition of a mapped resource. Typically, a mapped resource
 * is a file or a device: in both cases, only part of the resource is
 * mapped by each arena, this part is given by the offset_in_resource
 * field of the arena, and the size field of the arena.
 */
struct uvmm_mapped_resource
{
  /** Represent the maximum authrized ARENA_PROT_* for the arenas mapping
      it */
   __u32  allowed_access_rights;

  /** Some flags associated with the resource. Currently only
     MAPPED_RESOURCE_ANONYMOUS is supported */
     __u32  flags;

  /** List of arenas mapping this resource */
  struct uvmm_arena * list_arena;

  /**
   * MANDATORY Callback function called when a new arena is created,
   * which maps the resource. This callback is allowed to change the
   * following fields of the arena:
   *   - uvmm_set_ops_of_arena()
   */
  __u32 (*mmap)(struct uvmm_arena *);

  
  void *custom_data;
};

int uvmm_subsystem_setup();

struct uvmm_as *uvmm_create_empty_as( struct process*);

int uvmm_delete_as(struct uvmm_as * as);

int uvmm_map(struct uvmm_as * as,
		 __u32 * /*in/out*/uaddr, __u32 size,
		 __u32 access_rights,
		 __u32 flags,
		 struct uvmm_mapped_resource * resource,
		 __u64 offset_in_resource);

struct uvmm_mapped_resource *uvmm_get_mapped_resource_of_arena(struct uvmm_arena * arena);

__u32 uvmm_get_start_of_arena(struct uvmm_arena * arena);

__u64 uvmm_get_offset_in_resource(struct uvmm_arena * arena);

int uvmm_set_ops_of_arena(struct uvmm_arena * arena,
			   struct uvmm_arena_ops * ops);

int uvmm_get_prot_of_arena(struct uvmm_arena * arena);

int uvmm_get_flags_of_arena(struct uvmm_arena * arena);

int uvmm_init_heap(struct uvmm_as * as, __u32 heap_start);

int uvmm_lazy_loading(__u32 uaddr,
					      bool write_access,
					      bool user_access);

__u32 binfmt_elf32_map(struct  uvmm_as * dest_as,
				 const char * progname);

#endif


