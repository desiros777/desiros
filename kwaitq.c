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

#include <klibc.h>
#include <list.h>
#include <debug.h>
#include <kerrno.h>
#include <interrupt.h>

#include <kwaitq.h>


int kwaitq_init(struct kwaitq *kwq,
			  const char *name)
{
  memset(kwq, 0x0, sizeof(struct kwaitq));

 if (! name)
    name = "<unknown>";
  strzcpy(kwq->name, name, KWQ_DEBUG_MAX_NAMELEN);


  list_init_named(kwq->waiting_list,
		  prev_entry_in_kwaitq, next_entry_in_kwaitq);

  return OK;
}


int kwaitq_dispose(struct kwaitq *kwq)
{
  __u32 flags;
  int retval;

  disable_IRQs(flags);
  if (list_is_empty_named(kwq->waiting_list,
			  prev_entry_in_kwaitq, next_entry_in_kwaitq))
    retval = OK;
  else
    retval = -EBUSY;

  restore_IRQs(flags);
  return retval;
}


bool kwaitq_is_empty(const struct kwaitq *kwq)
{
  __u32 flags;
  int retval;

  disable_IRQs(flags);
  retval = list_is_empty_named(kwq->waiting_list,
			       prev_entry_in_kwaitq, next_entry_in_kwaitq);

  restore_IRQs(flags);
  return retval;  
}


int kwaitq_init_entry(struct kwaitq_entry *kwq_entry)
{
  memset(kwq_entry, 0x0, sizeof(struct kwaitq_entry));
  kwq_entry->proc = current ;
  return OK;
}


/** Internal helper function equivalent to sos_kwaitq_add_entry(), but
    without interrupt protection scheme */
inline static int
_kwaitq_add_entry(struct kwaitq *kwq,
		  struct kwaitq_entry *kwq_entry)
{
  struct kwaitq_entry *next_entry = NULL, *entry;
  int nb_entries;

  /* This entry is already added in the kwaitq ! */
  if(NULL == kwq_entry->kwaitq)debug();

  /* kwaitq_init_entry() has not been called ?! */
  if(NULL == kwq_entry->proc) debug();

  /* (Re-)Initialize wakeup status of the entry */
  kwq_entry->wakeup_triggered = false;
  kwq_entry->wakeup_status    = OK;

  
      /* Insertion in the list in FIFO order */
     
	/* Add the process in the list */
      list_add_tail_named(kwq->waiting_list, kwq_entry,
			    prev_entry_in_kwaitq, next_entry_in_kwaitq);
     

  kwq_entry->kwaitq = kwq;
  
  return OK;
}


int kwaitq_add_entry(struct kwaitq *kwq,
			       struct kwaitq_entry *kwq_entry)
{
  __u32 flags;
  int retval;

  disable_IRQs(flags);
  retval = _kwaitq_add_entry(kwq, kwq_entry);
  restore_IRQs(flags);

  return retval;
}


/** Internal helper function equivalent to kwaitq_remove_entry(),
    but without interrupt protection scheme */
inline static int
_kwaitq_remove_entry(struct kwaitq *kwq,
		     struct kwaitq_entry *kwq_entry)
{
  if(kwq_entry->kwaitq != kwq)debug();

  list_delete_named(kwq->waiting_list, kwq_entry,
		    prev_entry_in_kwaitq, next_entry_in_kwaitq);


  kwq_entry->kwaitq = NULL;
  return OK;
}


int kwaitq_remove_entry(struct kwaitq *kwq,
				  struct kwaitq_entry *kwq_entry)
{
  __u32 flags;
  int retval;

  disable_IRQs(flags);
  retval = _kwaitq_remove_entry(kwq, kwq_entry);
  restore_IRQs(flags);

  return retval;
}


int kwaitq_wait(struct kwaitq *kwq)
{
  __u32 flags;
  int retval;
  struct kwaitq_entry kwq_entry;

  kwaitq_init_entry(& kwq_entry);

  disable_IRQs(flags);

  retval = _kwaitq_add_entry(kwq, & kwq_entry);

 
 kwq_entry.proc->state = PROC_BLOCKED;
 schedule();


  /* Sleep delay elapsed ? */
  if (! kwq_entry.wakeup_triggered)
    {
      /* Yes (timeout occured, or wakeup on another waitqueue): remove
	 the waitq entry by ourselves */
      _kwaitq_remove_entry(kwq, & kwq_entry);
      retval = -EINTR;
    }
  else
    {
      retval = kwq_entry.wakeup_status;
    }
  
  restore_IRQs(flags);

  /* We were correctly awoken: position return status */
  return retval;
}


int kwaitq_wakeup(struct kwaitq *kwq,
			    unsigned int nb_process,
			    int wakeup_status)
{
  __u32 flags;

  disable_IRQs(flags);

  /* Wake up as much process waiting in waitqueue as possible (up to
     nb_process), scanning the list in FIFO/decreasing priority order
     (depends on the kwaitq ordering) */
  while (! list_is_empty_named(kwq->waiting_list,
			       prev_entry_in_kwaitq, next_entry_in_kwaitq))
    {
      struct kwaitq_entry *kwq_entry
	= list_get_head_named(kwq->waiting_list,
			      prev_entry_in_kwaitq, next_entry_in_kwaitq);

      /* Enough process woken up ? */
      if (nb_process <= 0)
	break;

      /*
       * Ok: wake up the process for this entry
       */

      /* Process already woken up ? */
      if (PROC_RUNNING == process_get_state(kwq_entry->proc))
	{
	  /* Yes => Do nothing because WE are that woken-up process. In
	     particular: don't call set_ready() here because this
	     would result in an inconsistent configuration (currently
	     running process marked as "waiting for CPU"...). */
	  continue;
	}
      else
	{
          kwq_entry->proc->state = PROC_READY ;
	  /* No => wake it up now. See schedule.c */
           reschedule(kwq_entry->proc->pid);
	}

      /* Remove this waitq entry */
      _kwaitq_remove_entry(kwq, kwq_entry);
      kwq_entry->wakeup_triggered = true;
      kwq_entry->wakeup_status    = wakeup_status;

      /* Next iteration... */
      nb_process --;
    }

  restore_IRQs(flags);

  return OK;
}


/* Internal function (callback for process subsystem) */
int kwaitq_change_priority(struct kwaitq *kwq,
				     struct kwaitq_entry *kwq_entry)
{
  /* Reorder the waiting list */
  _kwaitq_remove_entry(kwq, kwq_entry);
  _kwaitq_add_entry(kwq, kwq_entry);

  return OK;
}
