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


#ifndef _KLIBC_H_
#define _KLIBC_H_

#include <types.h>

void *memset (void * s, int c, int n);
void *memmove(void *, const void *, int);
void *memcpy(void *dst0, const void *src0, register unsigned int size);
char *strcpy(char *dest, const char *src);
int strcmp(const char *, const char *);
size_t strlen(const char* s);
char *strdup (const char *s);
char *strchrnul(const char *s, int c);
void itoa (char *buf, int base, int d);
void kprintf (const char *format, ...);

#endif

