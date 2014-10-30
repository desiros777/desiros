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

#define __GDT__
#include <gdt.h>
#include <klibc.h>
#include <multiboot.h>
/*
* 'init_gdt_desc' initializes a segment descriptor located gdt or ldt.
* 'desc' is the linear address of the descriptor to initialize.
*/
void init_gdt_desc(__u32 base, __u32 limite, __u8 acces, __u8 other,
		   struct gdtdesc *desc)
{
	desc->lim0_15 = (limite & 0xffff);
	desc->base0_15 = (base & 0xffff);
	desc->base16_23 = (base & 0xff0000) >> 16;
	desc->acces = acces;
	desc->lim16_19 = (limite & 0xf0000) >> 16;
	desc->other = (other & 0xf);
	desc->base24_31 = (base & 0xff000000) >> 24;
	return;
}

/*
* This function initializes the GDT after the kernel is loaded
 * In memory. A GDT is already operational, but it is one that
 * was initialised by the boot sector, and which is
 * was not the one we want.
 */
void init_gdt(void)
{
      
	default_tss.debug_flag = 0x00;
	default_tss.io_map = 0x00;
        default_tss.ss0 = 0x18;

     
         

	/* initializing segment descriptor */
	init_gdt_desc(0x0, 0x0, 0x0, 0x0, &kgdt[0]);
	init_gdt_desc(0x0, 0xFFFFF, 0x9B, 0x0D, &kgdt[1]);	/* code */
	init_gdt_desc(0x0, 0xFFFFF, 0x93, 0x0D, &kgdt[2]);	/* data */
	init_gdt_desc(0x0, 0x0, 0x97, 0x0D, &kgdt[3]);		/* stack */

	init_gdt_desc(0x0,  0xFFFFF, 0xFF, 0x0D, &kgdt[4]);	/* ucode */
	init_gdt_desc(0x0,  0xFFFFF, 0xF3, 0x0D, &kgdt[5]);	/* udata */
	init_gdt_desc(0x0, 0x0, 0xF7, 0x0D, &kgdt[6]);		/* ustack */

        init_gdt_desc((__u32) & default_tss, 0x67, 0xE9, 0x00, &kgdt[7]);	/* tss */

	/* initialization of the structure to GDTR */
	kgdtr.limite = GDTSIZE * 8;
	kgdtr.base = GDTBASE;

	/* copy of the GDT's address */
	memmove((char *) kgdtr.base, (char *) kgdt, kgdtr.limite);

	/* loading register GDTR */
	asm("lgdtl (kgdtr)");

	/* initialize segments */
	asm("   movw $0x10, %ax	\n \
            movw %ax, %ds	\n \
            movw %ax, %es	\n \
            movw %ax, %fs	\n \
            movw %ax, %gs	\n \
            ljmp $0x08, $next	\n \
            next:		\n");
}
