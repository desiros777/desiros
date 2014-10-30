#include <process.h>
#include <debug.h>
#include <list.h>
#include <physmem.h>

int sys_exit(){

       int nb_pt ;
        __u32 pd_paddr, kstack ;
       struct page_table * pt_to_del ;

        kstack = paging_virtual_to_physical(page_directory, current->kstack.esp0 - PAGE_SIZE);
        physmem_unref_physpage(kstack);

       list_foreach_named(current->list_pt, pt_to_del , nb_pt, prev, next)
       {
         /*Delete all page table of process*/
         physmem_unref_physpage((__u32)pt_to_del->pt);
       }

    while(! list_is_empty_named(current->list_pt,prev,next))
      {
       struct page_table * struct_pt = list_get_head_named(current->list_pt,prev,next);
       list_pop_head_named(current->list_pt,prev,next);
       list_delete_named(current->list_pt, struct_pt, prev, next);
       kvmm_free((__u32) struct_pt);

     }
     /*Delete directory of pages*/
     pd_paddr = current->regs.cr3 ;
      physmem_unref_physpage(pd_paddr);
 


        current->state = PROC_STOPPED ;
        switch_to_task(0, KERNELMODE);


       return 0 ;


}
