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
#define IDTSIZE		0xFF	/* max. of descriptors in the table */
#define IDTBASE		0x00000000	/* addr. physic of IDT */



#define INTGATE  0x8E00		/*  interruptions */
#define TRAPGATE 0xEF00		/* system calls */

/* Segment descriptor */
struct idtdesc {
	__u16 offset0_15;
	__u16 select;
	__u16 type;
	__u16 offset16_31;
} __attribute__ ((packed));

/* Register IDTR */
struct idtr {
	__u16 limite;
	__u32 base;
} __attribute__ ((packed));

struct idtr kidtr; /* Register IDTR */
struct idtdesc kidt[IDTSIZE]; /* Table IDT */

void init_idt_desc(__u16, __u32, __u16, struct idtdesc *);
void init_idt(void);
void init_pic(void);
