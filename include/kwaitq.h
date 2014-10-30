/* Copyright (C) 2004 David Decotigny

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
#ifndef _KWAITQ_H_
#define _KWAITQ_H_

#include <kerrno.h>
#include <process.h>



/**
 * @kwaitq.h
 *
 * Low-level functions to manage queues of threads waiting for a
 * resource. These functions are public, except
 * kwaitq_change_priority() that is a callback for the thread
 * subsystem. However, for higher-level synchronization primitives
 * such as mutex, semaphores, conditions, ... prefer to look at the
 * corresponding libraries.
 */

# define KWQ_DEBUG_MAX_NAMELEN 32

/* Forward declaration */
struct kwaitq_entry;



struct kwaitq
{
   char name[KWQ_DEBUG_MAX_NAMELEN];
  struct kwaitq_entry *waiting_list;
};


/**
 * Definition of an entry for a thread waiting in the waitqueue
 */
struct kwaitq_entry
{

  struct process     * proc;
  /** The kwaitqueue this entry belongs to */
  struct kwaitq *kwaitq;

  /** TRUE when somebody woke up this entry */
  bool wakeup_triggered;

  /** The status of wakeup for this entry. @see wakeup_status argument
      of kwaitq_wakeup() */
  int  wakeup_status;

  /** Other entries in this kwaitqueue */
  struct kwaitq_entry *prev_entry_in_kwaitq, *next_entry_in_kwaitq;

  /** Other entries for the process */
  struct kwaitq_entry *prev_entry_for_process, *next_entry_for_process;  
};

#endif
