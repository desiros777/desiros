#include <physmem.h>
#include <klibc.h>

#define __PLIST__

#include <process.h>
#include <multiboot.h>
#include <klibc.h>
#include <uvmm.h>
#include <debug.h>


static __u32 fill_stack(__u32 ustack, char const** argv[]) {

    char **ap;
    int argc, i;
    char **uparam;

    ap = argv;
	argc = 0;
      
         while (*ap++) 
		argc++;

             	uparam = (char**) kmalloc(sizeof(char*) * argc,0);

		for (i=0 ; i<argc ; i++) {
			ustack -= (strlen(argv[i]) + 1);
			strcpy((char*) ustack, argv[i]);
			uparam[i] = (char*) ustack;
		}
	
		ustack -= sizeof(char*);
		*((char**) ustack) = 0;

		for (i=argc-1 ; i>=0 ; i--) {	
			ustack -= sizeof(char*);
			*((char**) ustack) = uparam[i]; 
		}

		ustack -= sizeof(char*);	/* argv */
		*((char**) ustack) = (char*) (ustack + 4); 

		ustack -= sizeof(char*);	/* argc */
		*((int*) ustack) = argc; 

		ustack -= sizeof(char*);

		for (i=0 ; i<argc ; i++) 
			kfree(argv[i]);

		kfree(argv);
		kfree(uparam);
              
              return  ustack ;
}

void sys_exec(char * str, void const **argv )
{                 
	__u32 *pd ;
        __u32 ustack, start_uaddr;
	int pid;
     
        struct uvmm_as *new_as;

        __u32 kstack = kvmm_alloc(1, KVMM_MAP);
         

         pid = 1;
	while (p_list[pid].state != PROC_STOPPED && pid++ < MAXPID);
	if (p_list[pid].state != PROC_STOPPED ) {
		kprintf("PANIC: not enough slot for processes\n");
		return ;
	}

	num_proc++;
	p_list[pid].pid = pid;
        kprintf("exec PID %d\n", pid);
       new_as = uvmm_create_empty_as(&p_list[pid]);

#define DEFAULT_USER_STACK_SIZE (8 << 20)
	ustack = (0xFFBFFFFF - DEFAULT_USER_STACK_SIZE) + 1;
	          

  	/* Creation of the directory and page tables */
	pd = paging_pd_create();
   
        asm("mov %0, %%eax; mov %%eax, %%cr3"::"m"(pd));
	dev_zero_map(new_as, &ustack, DEFAULT_USER_STACK_SIZE,
				   P_READ | P_WRITE,/* PRIVATE */ 0);

if(argv != NULL){
 ustack = fill_stack( ustack, argv);
}	


       start_uaddr = binfmt_elf32_map(new_as, str);
	if (start_uaddr == (__u32)NULL)
	  {
	    uvmm_delete_as(new_as);
	    kfree((__u32)str);
	    return -8;
	  }

process_set_address_space(&p_list[pid], new_as);



	p_list[pid].regs.ss = 0x33;
	p_list[pid].regs.esp =  ustack  ;
	p_list[pid].regs.cs = 0x23;
	p_list[pid].regs.eip = start_uaddr;
	p_list[pid].regs.ds = 0x2B;
        p_list[pid].regs.es = 0x2B;
        p_list[pid].regs.fs = 0x2B;
        p_list[pid].regs.gs = 0x2B;
        p_list[pid].regs.cr3 = (__u32) pd;
        p_list[pid].regs.eflags = 0x0;
        p_list[pid].kstack.ss0 = 0x18;
        p_list[pid].kstack.esp0 = kstack  + PAGE_SIZE  ;

        p_list[pid].regs.eax = 0;
        p_list[pid].regs.ecx = 0;
        p_list[pid].regs.edx = 0;
        p_list[pid].regs.ebx = 0;

        p_list[pid].regs.ebp = 0;
        p_list[pid].regs.esi = 0;
        p_list[pid].regs.edi = 0;
        p_list[pid].state = PROC_READY;

     return;

}

