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
#include <idt.h>
#include <klibc.h>

void _asm_default_int(void);
void _asm_irq_0(void);
void _asm_exc_PF(void);
void _asm_syscalls(void);

 /*
  * 'init_idt_desc' initializes a segment descriptor located idt.
  * 'desc' is the linear address of the descriptor to initialize.
  * The 'type' argument must have for INTGATE value TRAPGATE
  * or TASKGATE.
  */
void init_idt_desc(__u16 select, __u32 offset, __u16 type, struct idtdesc *desc)
{
	desc->offset0_15 = (offset & 0xffff);
	desc->select = select;
	desc->type = type;
	desc->offset16_31 = (offset & 0xffff0000) >> 16;
	return;
}

/*
 * This function initializes the IDT after the kernel is loaded
 * in memory 
 */
void init_idt(void)
{
	int i;

	/* Initialization descriptor system default */
	for (i = 0; i < IDTSIZE; i++) 
	init_idt_desc(0x08, (__u32) _asm_default_int, INTGATE, &kidt[i]);

        init_idt_desc(0x08, (__u32) _asm_irq_0, INTGATE, &kidt[32]);
        init_idt_desc(0x08, (__u32) _asm_exc_PF, INTGATE, &kidt[14]);     /* #PF */
        /* 0x30 */
        init_idt_desc(0x08, (__u32) _asm_syscalls, TRAPGATE, &kidt[128]); 

	/* Initialization system default descriptor IDTR */
	kidtr.limite = IDTSIZE * 8;
	kidtr.base = IDTBASE;

	/* Copy of the IDT's address */
	memmove((char *) kidtr.base, (char *) kidt, kidtr.limite);

	/* Load register IDTR */
	asm("lidtl (kidtr)");
}
