/* Copyright (C) 2005  Thomas Petazzoni

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

#ifndef _PARTITION_H_
#define _PARTITION_H_

/**
 * @file part.h
 *
 * IBM PC partition "driver": in charge of registering each of the
 * partition of the given block device
 */

int part_detect (__u32 disk_class, __u32 disk_instance,
		 __u32 block_size, char *name);

int blockdev_register_partition(const char* name,__u32 device_class,
				__u32 device_instance,
				struct blockdev_instance * parent_bd,
				__u64 index_of_first_block,
				__u64 number_of_blocks,
				void * blockdev_instance_custom_data);
#endif 
