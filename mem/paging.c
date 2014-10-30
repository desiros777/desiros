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
#include <kmalloc.h>
#include <physmem.h>

#define _INITPD_ 
#include <mm.h>
#include <debug.h>
#include <process.h>
#include <list.h>

/** Structure of the x86 CR3 register: the Page Directory Base
    Register. See Intel x86 doc Vol 3 section 2.5 */
struct x86_pdbr
{
  __u32 zero1          :3; /* Intel reserved */
  __u32 write_through  :1; /* 0=write-back, 1=write-through */
  __u32 cache_disabled :1; /* 1=cache disabled */
  __u32 zero2          :7; /* Intel reserved */
  __u32 pd_paddr       :20;
} __attribute__ ((packed));



void paging_init()
{

         page_directory = (__u32*) physmem_ref_physpage_new(false);
         memset(page_directory, 0, sizeof(__u32) * 1024);
         
           __u32 address = 0;   

         __u32 dir_i;
        for( dir_i = 0; dir_i < 1024; dir_i++){
            __u32* table =  (__u32*) physmem_ref_physpage_new(false);
            page_directory[dir_i] = (__u32)table  | P_READ | P_WRITE| P_USER ;

              __u32 tbl_i ;
              for(tbl_i = 0; tbl_i < 1024; tbl_i++){
                 table[tbl_i] = address | P_READ | P_WRITE ;
                 address += 4096;
              }
        }


                 /* Page table mirroring magic trick !... */
	page_directory[1023] = ((__u32) page_directory | (P_PRESENT | P_WRITE));

        asm("   mov %0, %%eax    \n \
                mov %%eax, %%cr3 \n \
                mov %%cr4, %%eax \n \
		or %2, %%eax \n \
		mov %%eax, %%cr4 \n \
                mov %%cr0, %%eax \n \
                or %1, %%eax     \n \
                mov %%eax, %%cr0 \n \
                jmp 1f\n  \
                1: \n \
                movl $2f, %%eax\n \
                jmp *%%eax\n \
                2:\n" :: "m"(page_directory), "i" (PAGING_FLAG) , "i"(PSE_FLAG));


             
}

__u32 paging_map(__u32 virtual, __u32 physical, bool user)
{


        __u32 *pde;
	__u32 *pte;

        if(virtual & 0xfff)
        kprintf("Virtual address not page-aligned\n");

        if(physical & 0xff)
        kprintf("Physical address not page-aligned\n");


     physmem_ref_physpage_at(physical);

     
	pde = (__u32 *) (0xFFFFF000 | (((__u32) virtual & 0xFFC00000) >> 20));

	if ((*pde & P_PRESENT) == 0) 
		kprintf("PANIC: paging_map(): kernel page table not found !\n");
		
	

	/* Changing the entry in the page table */
	pte = (__u32 *) (0xFFC00000 | (((__u32) virtual & 0xFFFFF000) >> 10));
	*pte = ((__u32) physical) | 1 | 2 | (user ? 4: 0);



              
       
 
       return 0;
}

__u32 paging_unmap(__u32 virtual)
{

__u32 phys = paging_virtual_to_physical(page_directory,virtual);

         __u32 *pte;

        if(virtual & 0xfff){
        kprintf("Virtual address not page-aligned\n");
          return 1;
          }
  	
		pte = (__u32 *) (0xFFC00000 | (((__u32) virtual & 0xFFFFF000) >> 10));
		*pte = (*pte & (~P_PRESENT));
		flush_tlb_single(virtual);
	
          physmem_unref_physpage(phys);
	return 0;
}

int paging_unmap_interval(__u32 vaddr, __u32 size)
{
  int retval = 0;

  if (! IS_PAGE_ALIGNED(vaddr))
    return -1;
  if (! IS_PAGE_ALIGNED(size))
    return -1;

  for ( ;
	size >= PAGE_SIZE ;
	vaddr += PAGE_SIZE, size -= PAGE_SIZE)
    if (0 == paging_unmap(vaddr))
      retval += PAGE_SIZE;

  return retval;
}


__u32 paging_virtual_to_physical(__u32* page_directory, __u32 virtual){

#define	VADDR_PG_OFFSET(addr)	(addr) & 0x00000FFF

        __u32 *pde;		
	__u32 *pte;		

	pde = (__u32 *) (0xFFFFF000 | (((__u32) virtual & 0xFFC00000) >> 20));
	if ((*pde & P_PRESENT)) {
		pte = (__u32 *) (0xFFC00000 | (((__u32) virtual & 0xFFFFF000) >> 10));
		if ((*pte & P_PRESENT))
			return (__u32) ((*pte & 0xFFFFF000) + (VADDR_PG_OFFSET((__u32) virtual)));
	}

	return 0;
}




__u32*  paging_get_current_PD()
{
  __u32 * pd;
  asm volatile("movl %%cr3, %0\n": "=r"(pd));
  return pd ;
}

__u32 paging_load_PD(__u32  pd){

asm("mov %0, %%eax; mov %%eax, %%cr3"::"m"(pd));

    return 0;
}



__u32* paging_pd_create()
{
	__u32 *pd = NULL;
	int i;
    

	pd = (__u32*)physmem_ref_physpage_new(false);
   
       for (i = 0 ; i < 256; i++)
	pd[i] = page_directory[i] ;

        pd[1023] = (__u32) pd | (P_PRESENT | P_WRITE| P_READ) ;
 
	return pd;
}

int paging_pd_add_pt(char *vaddr, __u32 *pd)
{
	__u32 *pde;		
	__u32 *pt;		
	__u32 pg_paddr,pg_vaddr;
	int i;

	pde = (__u32 *) (0xFFFFF000 | (((__u32) vaddr & 0xFFC00000) >> 20));


	if ((*pde & P_PRESENT) == 0) {

  struct page_table *preallocated_pt = (struct page_table*)kvmm_cache_alloc(cache_pt, 0);

  pg_vaddr = kvmm_alloc(1, KVMM_MAP);


  if (pg_vaddr == (__u32)NULL){
    debug();
    return -3;
    }
  memset((void*)pg_vaddr, 0x0, PAGE_SIZE);

  
  /* Keep a reference to the underlying pphysical page... */
  pg_paddr =  paging_virtual_to_physical( page_directory,pg_vaddr);
  if(NULL == (void*)pg_paddr){

               debug();
               return 0 ;

  }
           
  physmem_ref_physpage_at(pg_paddr);

		/* It initializes the new page table */
		pt = (__u32 *) pg_vaddr;
		for (i = 0; i < 1024; i++)
			pt[i] = 0;



		/* The corresponding entry is added to the directory */
		*pde = (__u32) pg_paddr | (P_PRESENT | P_WRITE | P_USER );

      /*Save physical page */
      preallocated_pt->pt = pg_paddr;
      list_add_head_named(current->list_pt,preallocated_pt , prev, next);
		
	}


	return 0;
}


