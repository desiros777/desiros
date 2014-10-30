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


#ifndef _GDT_H
#define _GDT_H

#include <types.h>
#define GDTSIZE		0xFF	/* nomber of max. descriptors */
#define GDTBASE		0x00000800	/* addr. physic of gdt */

/* Segments: */
#define UNUSED_SEG	0 /* Unused segment.      */
#define KERNEL_CODE_SEG	1 /* Kernel code segment. */
#define KERNEL_DATA_SEG	2 /* Kernel data segment. */
#define USER_CODE_SEG	3 /* User code segment.   */
#define USER_DATA_SEG	4 /* User data segment.   */
#define NUM_SEGS	5 /* Number of segments.  */


/* Segment descriptor */
struct gdtdesc {
	__u16 lim0_15;
	__u16 base0_15;
	__u8 base16_23;
	__u8 acces;
	__u8 lim16_19:4;
	__u8 other:4;
	__u8 base24_31;
} __attribute__ ((packed));

/* Reg GDTR */
struct gdtr {
	__u16 limite;
	__u32 base;
} __attribute__ ((packed));

struct tss {
	__u16 previous_task, __previous_task_unused;
	__u32 esp0;
	__u16 ss0, __ss0_unused;
	__u32 esp1;
	__u16 ss1, __ss1_unused;
	__u32 esp2;
	__u16 ss2, __ss2_unused;
	__u32 cr3;
	__u32 eip, eflags, eax, ecx, edx, ebx, esp, ebp, esi, edi;
	__u16 es, __es_unused;
	__u16 cs, __cs_unused;
	__u16 ss, __ss_unused;
	__u16 ds, __ds_unused;
	__u16 fs, __fs_unused;
	__u16 gs, __gs_unused;
	__u16 ldt_selector, __ldt_sel_unused;
	__u16 debug_flag, io_map;
} __attribute__ ((packed));

#ifdef __GDT__
	struct gdtdesc kgdt[GDTSIZE];	/* GDT */
	struct gdtr kgdtr;		/* GDTR */
	struct tss default_tss;
#else
	extern struct gdtdesc kgdt[];
	extern struct gdtr kgdtr;
	extern struct tss default_tss;
#endif
void init_gdt(void);

#endif
