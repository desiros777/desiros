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

#include "io.h"

// This function initiates the programmable interrupt controler
void init_pic(void)
{
	/* ICW1 initialization*/
	outb(0x20, 0x11);
	outb(0xA0, 0x11);

	/* ICW2 initialization*/
	outb(0x21, 0x20);	
	outb(0xA1, 0x70);	

	/* ICW3 initialization */
	outb(0x21, 0x04);
	outb(0xA1, 0x02);

	/* ICW4 initialization*/
	outb(0x21, 0x01);
	outb(0xA1, 0x01);

	/* interrupt masking */
	outb(0x21, 0x0);
	outb(0xA1, 0x0);
}


