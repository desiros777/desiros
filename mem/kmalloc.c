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
#include <debug.h>
#include <macros.h>
#include <types.h>
#include <physmem.h>
#include <kvmm.h>
#include <mm.h>
#include <kmalloc.h>




static struct {
  const char             *name;
  __u32             object_size;
  __u32            pages_per_slab;
  struct kslab_cache *cache;
} kmalloc_cache[] =
  {
    { "kmalloc 8B objects",     8,     1  },
    { "kmalloc 16B objects",    16,    1  },
    { "kmalloc 32B objects",    32,    1  },
    { "kmalloc 64B objects",    64,    1  },
    { "kmalloc 128B objects",   128,   1  },
    { "kmalloc 256B objects",   256,   2  },
    { "kmalloc 1024B objects",  1024,  2  },
    { "kmalloc 2048B objects",  2048,  3  },
    { "kmalloc 4096B objects",  4096,  4  },
    { "kmalloc 8192B objects",  8192,  8  },
    { "kmalloc 16384B objects", 16384, 12 },
    { NULL, 0, 0, NULL }
  };

int kmalloc_setup()
{
  int i;
  for (i = 0 ; kmalloc_cache[i].object_size != 0 ; i ++)
    {
                  
      struct  kslab_cache *new_cache;
      new_cache = kvmm_cache_create(kmalloc_cache[i].name,
					kmalloc_cache[i].object_size,
					kmalloc_cache[i].pages_per_slab,
					0,
					KSLAB_CREATE_MAP
					);
      if(new_cache == NULL)
            debug();
      kmalloc_cache[i].cache = new_cache;
    }
  return 0;
}

__u32  kmalloc(__u32 size, __u32 flags)
{
  /* Look for a suitable pre-allocated kmalloc cache */
  int i;
  if(size < 0)
      debug();

  for (i = 0 ; kmalloc_cache[i].object_size != 0 ; i ++)
    {
      if (kmalloc_cache[i].object_size >= size)
	return kvmm_cache_alloc(kmalloc_cache[i].cache, (flags
				     & KMALLOC_ATOMIC)?
				    KSLAB_ALLOC_ATOMIC:0);
    }

  /* none found yet => we directly use the kmem_vmm subsystem to
     allocate whole pages */
  return kvmm_alloc(PAGE_ALIGN_SUP(size) / PAGE_SIZE,( (flags
			       & KMALLOC_ATOMIC)?
			      KVMM_ATOMIC:0)
			    | KVMM_MAP
			    );
}


int kfree(__u32 vaddr)
{
  /* The trouble here is that we aren't sure whether this object is a
     slab object in a pre-allocated kmalloc cache, or an object
     directly allocated as a kmem_vmm region. */
  
  /* We first pretend this object is allocated in a pre-allocated
     kmalloc cache */
  if (!kvmm_cache_free(vaddr))
    return 0; /* Great ! We guessed right ! */
    
  /* Here we're wrong: it appears not to be an object in a
     pre-allocated kmalloc cache. So we try to pretend this is a
     kmem_vmm area */
  return kvmm_free(vaddr);
}
