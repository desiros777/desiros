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


#include <multiboot.h>
 
#include <klibc.h> 
#include <gdt.h> 
#include <idt.h> 
#include <io.h>  
#include <mm.h>    
#include <kmalloc.h>
#include <console.h>
#include <physmem.h>
#include <kvmm.h>
#include <assert.h>
#include <segment.h>
#include <debug.h>
#include <process.h>
#include <list.h>
#include <uvmm.h>
#include <ide.h>
#include <vfs.h>
#include <fs/devfs.h>
#include <kfcntl.h>

#define ok "...[OK]\n"
 /* Check if the bit BIT in FLAGS is set. */
#define CHECK_FLAG(flags,bit)   ((flags) & (1 << (bit)))

     
     /* Forward declarations. */
     void main_kernel (unsigned long magic, unsigned long addr);


void main_kernel (unsigned long magic, unsigned long addr)
     {
   
	__u32 kernel_base_paddr,kernel_top_paddr;
	init_console();
       
	multiboot_info_t *mbi;      
     
	/* Am I booted by a Multiboot-compliant boot loader? */
	if (magic != MULTIBOOT_BOOTLOADER_MAGIC)
	{
	kprintf ("Invalid magic number: 0x%x\n", (unsigned) magic);
	return;
	}
     
	/* Set MBI to the address of the Multiboot information structure. */
	mbi = (multiboot_info_t *) addr;
     
	/* Are mem_* valid? */
	if (CHECK_FLAG (mbi->flags, 0))
	kprintf ("RAM detected : %uKiB (lower), %uKiB (upper)\n",
                 (unsigned) mbi->mem_lower, (unsigned) mbi->mem_upper);

	kprintf("kernel: loading GDT ..................");
	init_gdt();
	kprintf(ok);

     if( physmem_setup((mbi->mem_upper<<10)+ (1<<20)  ,&kernel_base_paddr,
                                        &kernel_top_paddr))
            kprintf("Could not setup paged memory mode\n");

	paging_init();
	kprintf("kernel: Paging enable\n");

	kvmm_setup(kernel_base_paddr,
                             kernel_top_paddr ,
                             stack_bottom,
                             stack_bottom + stack_size);

	cli;

	kprintf("kernel: Loading IDT ..................");
	init_idt();
	kprintf(ok);

	//initiate programmable interrupt controler
	init_pic();
	kprintf("kernel: pic configured\n");

	kprintf("kernel: loading Task Register ........");
	asm("	movw $0x38, %ax; ltr %ax");
	kprintf(ok);

	kmalloc_setup();

	kprintf("kernel: User virtual memory management");
	uvmm_subsystem_setup();

        dev_zero_subsystem_setup();
        kprintf(ok);

	kprintf("kernel: Initialize Virtual File System");
	vfs_init();
	kprintf(ok);
	kprintf("kernel: Initialize devfs .............");
	devfs_init();
	kprintf(ok);
	
        // test kmalloc
	pci_scan();

	kprintf("kernel: Register devices ... ");
	ide_subsystem_setup ();

        kprintf("kernel: Initialize ext2 File System...");
        ext2_init();
	kprintf(ok);

        // mounting device file system
	vfs_mount(NULL, "dev", "DevFS");
        // mounting virtual file system
	vfs_mount("/dev/hda1", "core", "EXT2");

	kprintf("Switching to user task (ring3 mode)\n");

                current = &p_list[0];
		current->pid = 0;
		current->state = PROC_READY;
                current->regs.cr3 = (__u32) page_directory;



		
 char *prog1_name = "/core/boot/myprog3";
 char *prog2_name = "myprog2";

 //sys_exec(prog2_name);

 sys_exec(prog1_name,NULL);
 
 while(1);

   sti;

}



     
    
