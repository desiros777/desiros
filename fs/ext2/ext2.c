/**
 * @file ext2.c
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 3 of
 * the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details at
 * http://www.gnu.org/copyleft/gpl.html
 *
 * You should have received a copy of the GNU General Public License 
 * along with this program; if not, see <http://www.gnu.org/licenses>.
 * EXT2 File System.
 */

#include <fs/ext2.h>
#include <kmalloc.h>

#include "ext2_internal.h"

static file_system_t ext2_fs = {.name="EXT2", .unique_inode=1, .mount=mount_EXT2, .umount=umount_EXT2};

static int ceil(float n) {
  return (n == (int)n) ? n : (int)(n+(n>=0));
} 

/*
 * Register this FS.
 */
void ext2_init() {
	vfs_register_fs(&ext2_fs);
}

static void read_group_desc_table(ext2_fs_instance_t *node) {
	int b = node->superblock.s_first_data_block + 1;
	node->n_groups = ceil((float)node->superblock.s_blocks_count / (float)node->superblock.s_blocks_per_group);
	node->group_desc_table = kmalloc(sizeof(struct ext2_group_desc) * node->n_groups,0);

	node->read_data(node->super.device, node->group_desc_table, sizeof(struct ext2_group_desc) * node->n_groups, b * (1024 << node->superblock.s_log_block_size));


        node->group_desc_table_internal = kmalloc(sizeof(struct ext2_group_desc) * node->n_groups ,0);
	int i;
	for (i = 0; i < node->n_groups; i++) {
		node->group_desc_table_internal[i].inode_bitmap = kmalloc(node->superblock.s_inodes_per_group / 8,0);
		int addr_bitmap = node->group_desc_table[i].bg_inode_bitmap;
		node->read_data(node->super.device, node->group_desc_table_internal[i].inode_bitmap, node->superblock.s_inodes_per_group / 8, addr_bitmap * (1024 << node->superblock.s_log_block_size));

		
	}
	
}

static void show_info(ext2_fs_instance_t *node) {
	kprintf("\nNumber of inodes : %d\n", node->superblock.s_inodes_count);
	kprintf("Size of an inode : %d\n", node->superblock.s_inode_size);
	kprintf("Free blocks : %d\n", node->superblock.s_free_blocks_count);
	kprintf("Free inodes  : %d\n", node->superblock.s_free_inodes_count);
	kprintf("Block size : %d\n", 1024 << node->superblock.s_log_block_size);
	kprintf("Blocks per group : %d\n", node->superblock.s_blocks_per_group);
}

/*
 * Init the EXT2 driver for a specific devide.
 */
fs_instance_t* mount_EXT2(open_file_descriptor* ofd) {
	kprintf("mounting EXT2 \n");
	ext2_fs_instance_t *node = kmalloc(sizeof(ext2_fs_instance_t),0);

	node->super.fs = &ext2_fs;
        
	node->read_data = ((blkdev_interfaces*)(ofd->i_fs_specific))->read;
	node->write_data = ((blkdev_interfaces*)(ofd->i_fs_specific))->write;
          
	node->super.mknod = ext2_mknod;
	node->super.mkdir = ext2_mkdir;
	node->super.rename = ext2_rename;
	node->super.setattr = ext2_setattr;
	node->super.unlink = ext2_unlink;
	node->super.rmdir = ext2_rmdir;
	node->super.truncate = ext2_truncate;
	node->super.lookup = ext2_lookup;
	node->super.getroot = ext2_getroot;
	//node->super.stat = ext2_stat;
	node->super.stat = NULL;
	node->super.device = ofd;
 
  node->read_data(node->super.device, &(node->superblock), sizeof(struct ext2_super_block), 1024);
   show_info(node);


  read_group_desc_table(node);


	node->root = init_rootext2fs(node);

  

	return (fs_instance_t*)node;
}

void umount_EXT2(fs_instance_t *node) {
	kfree(node);
}

