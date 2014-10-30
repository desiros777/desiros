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

#ifndef _MACROS_H_
#define _MACROS_H_

/** Align on a boundary (MUST be a power of 2), so that return value <= val */
#define ALIGN_INF(val,boundary) \
  (((unsigned)(val)) & (~((boundary)-1)))
 
/** Align on a boundary (MUST be a power of 2), so that return value >= val */
#define ALIGN_SUP(val,boundary) \
  ({ unsigned int __bnd=(boundary); \
     (((((unsigned)(val))-1) & (~(__bnd - 1))) + __bnd); })

/** Check whether val is aligned on a boundary (MUST be a power of 2) */
#define IS_ALIGNED(val,boundary) \
  ( 0 == (((unsigned)(val)) & ((boundary)-1)) )

/**
 * @return TRUE if val is a power of 2.
 * @note val is evaluated multiple times
 */
#define IS_POWER_OF_2(val) \
  ((((val) - 1) & (val)) == 0)

#endif /* _MACROS_H_ */
