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


#ifndef _MM_H_
#define _MM_H_
#include <types.h>



__u32 *page_directory ;



#define	PAGING_FLAG 	0x80000000	/* CR0 - bit 31 */
#define PSE_FLAG	0x00000010	/* CR4 - bit 4  */

// Page present flag.
#define P_PRESENT       0x01

#define P_READ  (1<<0)  
// Page writeable flag.
#define P_WRITE         0x02
// Page at user privilege level.
#define P_USER          0x04
// Page accessed flag.
#define P_ACCESSED      0x20
// Page dirty flag (the page has been written).
#define P_DIRTY         0x40


#define USER_OFFSET  (0x40000000)   /* 1GB (must be 4MB-aligned) */

#define PAGING_TOP_USER_ADDRESS  (0xFFFFFFFF) /* 4GB - 1B */
#define PAGING_USER_SPACE_SIZE   (0xc0000000) /* 3GB */
  	
#define PAGING_IS_USER_AREA(base_addr, length) \
  ( (USER_OFFSET <= (base_addr))  \
      && ((length) <= PAGING_USER_SPACE_SIZE) )
 /* Page tables area start address. */
#define PAGE_TABLE_MAP   (0xFFC00000) 

/** Physical pages should be immediately mapped */
#define KVMM_MAP    (1<<0)
/** Allocation should either success or fail, without blocking */
#define KVMM_ATOMIC (1<<1)
/*
 * Flags for sos_kmem_cache_alloc()
 */
/** Allocation should either succeed or fail, without blocking */
#define KSLAB_ALLOC_ATOMIC (1<<0)
/** The slabs should be initially mapped in physical memory */
#define KSLAB_CREATE_MAP  (1<<0)
/** The object should always be set to zero at allocation (implies
    SOS_KSLAB_CREATE_MAP) */
#define KSLAB_CREATE_ZERO (1<<1)

/** The maximum allowed pages for each slab */
#define MAX_PAGES_PER_SLAB 32 /* 128 kB */


/* The base and top virtual addresses covered by the kernel allocator */
#define KVMM_BASE 0x4000 /* 16kB */
#define KVMM_TOP  0x40000000 


// Flush a single entry into the TLB cache (address translation).
 // \param addr
//     The virtual address of the page that must be invalidated.
#define flush_tlb_single(addr) \
         __asm__ __volatile__("invlpg %0": :"m" (*(char *) addr))

// Flush every entry into the TLB cache (address translation).
#define flush_tlb_all() \
         do {                                                            \
                unsigned int tmpreg;                                    \
                                                                         \
                __asm__ __volatile__(                                   \
                         "movl %%cr3, %0;  # flush TLB \n"               \
                        "movl %0, %%cr3;              \n"               \
                         : "=r" (tmpreg)                                 \
                         :: "memory");                                   \
         } while (0)


extern  __u32 zero_page;

void paging_init();

__u32 paging_map(__u32 virtual, __u32 physical, bool user);
__u32 paging_unmap(__u32 virtual);
__u32 paging_virtual_to_physical(__u32* page_directory, __u32 virtual);
__u32*  paging_get_current_PD();
__u32 paging_load_PD(__u32  pd);
int paging_pd_add_pt(char *vaddr, __u32 *pd);
#endif


