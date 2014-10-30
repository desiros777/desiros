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


#include <klibc.h>
#include <process.h>
#include <klibc.h>
#include <uvmm.h>
#include <debug.h>


int  process_set_address_space(struct process *proc,
					struct uvmm_as *new_as)
{
  if (proc->address_space)
    {
      while(1);
      int retval = uvmm_delete_as(proc->address_space);
      if (0 != retval)
	return retval;
    }

  proc->address_space = new_as;

  return 0 ;
}


struct uvmm_as *
process_get_address_space(const struct process *proc)
{
  return proc->address_space;
}


int process_get_state(const struct process *proc){

  return proc->state ;

}



