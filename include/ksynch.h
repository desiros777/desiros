/* 

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
#ifndef _KSYNCH_H_
#define _KSYNCH_H_


/**
 * @file synch.h
 *
 * Common kernel synchronisation primitives.
 */


#include <kwaitq.h>


/* ====================================================================
 * Kernel semaphores, NON-recursive
 */


/**
 * The structure of a (NON-RECURSIVE) kernel Semaphore
 */
struct ksema
{
  int value;
  struct kwaitq kwaitq;
};

/**
 * The structure of a (NON-RECURSIVE) kernel Mutex
 */
struct kmutex
{
  struct process *owner;
  struct kwaitq  kwaitq;
};


#endif
