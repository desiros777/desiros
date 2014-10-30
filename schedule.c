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

#include <types.h>
#include <gdt.h>
#include <process.h>
#include <mm.h>

void switch_to_task(int n, int mode)
{
        __u32 kesp, eflags;
        __u16 kss, ss, cs;

        current = &p_list[n];
        current->state = PROC_RUNNING;

        /* load tss */
        default_tss.ss0 = current->kstack.ss0;
        default_tss.esp0 = current->kstack.esp0;
              
        /*
         Stacks ss register, esp, eflags, cs and eip has the necessary
         switching. Then the do_switch () function restores
         registers, the page table of the new process and switches
         with iret.
         */
        ss = current->regs.ss;
        cs = current->regs.cs;
        eflags = (current->regs.eflags | 0x200) & 0xFFFFBFFF;

        if (mode == USERMODE) {
                kss = current->kstack.ss0;
                kesp = current->kstack.esp0;
        } else {                /*KERNELMODE */
                kss = current->regs.ss;
                kesp = current->regs.esp;
        }

        asm("   mov %0, %%ss; \
                mov %1, %%esp; \
                cmp %[KMODE], %[mode]; \
                je next; \
                push %2; \
                push %3; \
                next: \
                push %4; \
                push %5; \
                push %6; \
                push %7; \
                ljmp $0x08, $do_switch"
                :: \
                "m"(kss), \
                "m"(kesp), \
                "m"(ss), \
                "m"(current->regs.esp), \
                "m"(eflags), \
                "m"(cs), \
                "m"(current->regs.eip), \
                "m"(current), \
                [KMODE] "i"(KERNELMODE), \
                [mode] "g"(mode)
            );
}


void schedule(void) 
{
         struct process *p;         
	__u32* stack_ptr;
        int i,newpid ;

	asm("mov (%%ebp), %%eax; mov %%eax, %0" : "=m" (stack_ptr) : );

        current->state = PROC_READY;

	if (!num_proc) {
		return;
	}

	/* If there is a single process, let it run */
	else if (num_proc == 1 && current->pid != 0) {
		return;
	}

	/* If there are at least two processes, moves following */
	else {
		/* Save the registers of the current process*/
		current->regs.eflags = stack_ptr[16];
                current->regs.cs = stack_ptr[15];
                current->regs.eip = stack_ptr[14];
                current->regs.eax = stack_ptr[13];
                current->regs.ecx = stack_ptr[12];
                current->regs.edx = stack_ptr[11];
                current->regs.ebx = stack_ptr[10];
                current->regs.ebp = stack_ptr[8];
                current->regs.esi = stack_ptr[7];
                current->regs.edi = stack_ptr[6];
                current->regs.ds = stack_ptr[5];
                current->regs.es = stack_ptr[4];
                current->regs.fs = stack_ptr[3];
                current->regs.gs = stack_ptr[2];

		current->regs.esp = stack_ptr[17];
		current->regs.ss = stack_ptr[18];
		
		
	        if (current->regs.cs != 0x08) {
                        current->regs.esp = stack_ptr[17];
                        current->regs.ss = stack_ptr[18];
                } else {        /* Interruption during a system call */
                        current->regs.esp = stack_ptr[9] + 12;
                        current->regs.ss = default_tss.ss0;
                }

                /* Save tss */
                current->kstack.ss0 = default_tss.ss0;
                current->kstack.esp0 = default_tss.esp0;
                
	}

        newpid = 0;
	for (i = current->pid + 1; i < MAXPID && newpid == 0; i++) {
		if (p_list[i].state == PROC_READY )
			newpid = i;
	}

	if (!newpid) {
		for (i = 1; i < current->pid && newpid == 0; i++) {
			if (p_list[i].state == PROC_READY)
				newpid = i;
		}
	}

	p = &p_list[newpid];


	if (p->regs.cs != 0x08)
		switch_to_task(p->pid, USERMODE);
	else
		switch_to_task(p->pid, KERNELMODE);

                        
}


void reschedule(int pid) 
{

          struct process *p;         
	__u32* stack_ptr;
	
  

	asm("mov (%%ebp), %%eax; mov %%eax, %0" : "=m" (stack_ptr) : );


	if (!num_proc) {
		return;
	}

	/* If there is a single process, let it run */
	else if (num_proc == 1 && current->pid != 0) {
		return;
	}

	/* If there are at least two processes, moves following */
	else {
		/* Save the registers of the current process*/
		current->regs.eflags = stack_ptr[16];
                current->regs.cs = stack_ptr[15];
                current->regs.eip = stack_ptr[14];
                current->regs.eax = stack_ptr[13];
                current->regs.ecx = stack_ptr[12];
                current->regs.edx = stack_ptr[11];
                current->regs.ebx = stack_ptr[10];
                current->regs.ebp = stack_ptr[8];
                current->regs.esi = stack_ptr[7];
                current->regs.edi = stack_ptr[6];
                current->regs.ds = stack_ptr[5];
                current->regs.es = stack_ptr[4];
                current->regs.fs = stack_ptr[3];
                current->regs.gs = stack_ptr[2];

		current->regs.esp = stack_ptr[17];
		current->regs.ss = stack_ptr[18];
		
		
	        if (current->regs.cs != 0x08) {
                        current->regs.esp = stack_ptr[17];
                        current->regs.ss = stack_ptr[18];
                } else {        /* Interruption during a system call */
                        current->regs.esp = stack_ptr[9] + 12;
                        current->regs.ss = default_tss.ss0;
                }

                /* Save tss */
                current->kstack.ss0 = default_tss.ss0;
                current->kstack.esp0 = default_tss.esp0;

           }


        p = &p_list[pid];


	if (p->regs.cs != 0x08)
		switch_to_task(p->pid, USERMODE);
	else
		switch_to_task(p->pid, KERNELMODE);
}
