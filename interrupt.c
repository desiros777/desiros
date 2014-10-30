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
#include <schedule.h>
#include <uvmm.h>
#include <mm.h>



void isr_default_int(void)
{
	kprintf("interrupt\n");
}

void isr_clock_int(void)
{
	static int tic = 0;
	static int sec = 0;
	tic++;
	if (tic % 100 == 0) {
		sec++;
		tic = 0;
              schedule();
	}
	
}


void isr_page_fault(void)
{		
               __u32 faulting_vaddr, errcode,eip;
	
	

 	asm(" 	movl 60(%%ebp), %%eax	\n \
    		mov %%eax, %0		\n \
		mov %%cr2, %%eax	\n \
		mov %%eax, %1		\n \
 		movl 56(%%ebp), %%eax	\n \
    		mov %%eax, %2"
		: "=m"(eip), "=m"(faulting_vaddr), "=m"(errcode));


    paging_pd_add_pt((char*)faulting_vaddr, (__u32*) current->regs.cr3);

    if (0 != uvmm_lazy_loading(faulting_vaddr,
					      errcode & (1 << 1),
					      true)){

	kprintf("DEBUG: isr_PF_exc(): #PF on eip: %x. cr2: %x code: %x\n", eip, faulting_vaddr, errcode);
           while(1);
         }
        
       

        
}

