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


#ifndef _IO_H_
#define _IO_H_


#include <types.h>

/* disable the interrupt */
#define cli asm("cli"::)

/* enable interrupt */
#define sti asm("sti"::)


#define outb(port,value) \
	asm volatile ("outb %%al, %%dx" :: "d" (port), "a" (value));


#define outbp(port,value) \
	asm volatile ("outb %%al, %%dx; jmp 1f; 1:" :: "d" (port), "a" (value));


#define inb(port) ({    \
	unsigned char _v;       \
	asm volatile ("inb %%dx, %%al" : "=a" (_v) : "d" (port)); \
        _v;     \
})


#define outw(port,value) \
	asm volatile ("outw %%ax, %%dx" :: "d" (port), "a" (value));


#define inw(port) ({		\
	__u16 _v;			\
	asm volatile ("inw %%dx, %%ax" : "=a" (_v) : "d" (port));	\
        _v;			\
})


static inline void outl(__u16 port, __u32 val)
{
    __asm__ volatile ("outl %0, %1" : : "a"(val), "d"(port));
}

static inline __u32 inl(__u16 port)
{
    __u32 ret_val;
    __asm__ volatile ("inl %1, %0" : "=a" (ret_val) : "d"(port));
    return ret_val;
}

#endif

