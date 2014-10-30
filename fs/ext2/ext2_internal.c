/* Copyright (C) 2007 Desiros kernel TEAM
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


#include "ext2_internal.h"
#include <klibc.h>
#include <kmalloc.h>
#include <kdirent.h>
#include <kerrno.h>
#include <time.h>
#include <debug.h>

struct _open_file_operations_t ext2fs_fops = {.write = ext2_write, .read = ext2_read, .seek = ext2_seek, .ioctl = NULL, .open = NULL, .close = ext2_close, .readdir = NULL};

static __u32 addr_inode_data(ext2_fs_instance_t *instance, int inode, int n_blk);
static __u32 alloc_block(ext2_fs_instance_t *instance);
struct ext2_inode* read_inode(ext2_fs_instance_t *instance, int inum);
#define max(a,b) ((a) > (b) ? (a) : (b))

void
get_block (ext2_fs_instance_t *instance,unsigned long bnum, unsigned char *block)
{
    __u32 block_size = (1024 << instance->superblock.s_log_block_size);
    
        uint64 to_seek = bnum;
        to_seek = (to_seek * block_size) ; 
        instance->read_data(instance->super.device, block,block_size ,to_seek);
}

__u32 get_data_block ( ext2_fs_instance_t *instance, struct ext2_inode *inode, 
      int n /* requested file data block */) 
{
    __u32 addr;
    __u32 block_size = (1024 << instance->superblock.s_log_block_size);
    unsigned char *ind_block  = (unsigned char *)kmalloc(block_size,0); 
    unsigned char *dind_block = (unsigned char *)kmalloc(block_size,0); 
    unsigned char *tind_block = (unsigned char *)kmalloc(block_size,0); 

    unsigned int size; /* size of file in blocks */
    unsigned int *p1, *p2, *p3;  

 int ClustByteShift = instance->superblock.s_log_block_size + 10;

 int PtrsPerBlock1 = 1 << (ClustByteShift - 2);
 int PtrsPerBlock2 = 1 << ((ClustByteShift - 2) * 2);
 int PtrsPerBlock3 = 1 << ((ClustByteShift - 2) * 3);



if (inode->i_size == 0)
        size = 0; 
    else 
        size = 1 + ( (inode->i_size - 1) / block_size );

    if (size  == 0) 
        return -1; 

    if ( (n < 0)  || (n >=size)) 
         return -1; 

  /* direct blocks */
        if (n < EXT2_NDIR_BLOCKS) {
                 // direct blocks
          addr = inode->i_block[n];
          return addr * block_size ;
       } 
    
        /* indirect blocks */
        n -= EXT2_NDIR_BLOCKS;
        if (n < PtrsPerBlock1) {
           get_block (instance, inode->i_block[EXT2_IND_BLOCK], ind_block);
               p1 = (unsigned int *) &ind_block;
               addr = p1[n];
               return addr * block_size ;
        }
       
        /* double indirect blocks */
        n -= PtrsPerBlock1;
        if (n < PtrsPerBlock2) {
             get_block (instance,inode->i_block[EXT2_DIND_BLOCK], dind_block);
                 p2 = (unsigned int *) &dind_block;
           

               get_block ( instance,p2[n / PtrsPerBlock1] , ind_block);
               p1 = (unsigned int *) &ind_block;
               addr = p1[n % PtrsPerBlock1] ;
            
           return addr * block_size ;
            
        }
       
             /* triple indirect block */
         n -= PtrsPerBlock2;
        if (n < PtrsPerBlock3) {

                 get_block (instance,inode->i_block[EXT2_TIND_BLOCK], tind_block);
                 p3 = (unsigned int *) &tind_block;
           
                 get_block (instance,p3[n/ PtrsPerBlock2], dind_block);
                 p2 = (unsigned int *) &dind_block;

           
             get_block (instance,p2[n % PtrsPerBlock2], ind_block);
                 p1 = (unsigned int *) &ind_block;

            addr = p1[n % PtrsPerBlock1];
    
            return addr * block_size ;
        }
       
       
        /* File too big, can not handle */
        kprintf("ext2 ERROR, file too big\n");
         
    return -1; 


}

int find_dir_entry(ext2_fs_instance_t *instance,struct ext2_inode *dir_inode, const char *name, 
           struct ext2_directory *diren)
{
        
    struct ext2_directory  *dep; 
        int i, count; 
        char entry_name[256];
        __u32 block_addr ;
 __u32 block_size = (1024 << instance->superblock.s_log_block_size);

    unsigned char *block =(unsigned char*) kmalloc(block_size,0);
    int logical; 
    

    logical  = 0; 
       block_addr = get_data_block (instance,dir_inode, logical);
       instance->read_data(instance->super.device, block,block_size ,block_addr);

        i = 0;
    count = 0; 
         
        while (1) {
        dep = (struct  ext2_directory*) (block + i);

                memmove(entry_name, dep->name, dep->name_len);
                entry_name[dep->name_len] = '\0';


                if (strcmp(entry_name, name)  == 0) {
            memmove( (void *) diren, (void *) dep, 
                 sizeof (struct ext2_directory));
                        return (0); 

        }
                i += dep->rec_len;
        count += dep->rec_len;

        if (count >= dir_inode->i_size) 
            break; 
        
                if (i >= block_size) {
            i = i % block_size; 
            logical++; 
            block_addr = get_data_block (instance,dir_inode, logical);
            instance->read_data(instance->super.device, block,block_size ,block_addr);
        }
        }
        return (-1);
}


static struct directories_t * readdir_inode(ext2_fs_instance_t *instance, int inode,const char *name) {
	struct directories_t * dir_result = (struct directories_t*)kmalloc(sizeof(struct directories_t),0);

        struct ext2_directory  dep;
	struct ext2_inode* dir_inode = read_inode(instance, inode) ;
        
          find_dir_entry(instance, dir_inode,name, &dep);

	dir_result->dir = &dep;
	dir_result->next = NULL;

	

	return dir_result;
}

int getinode_from_name(ext2_fs_instance_t *instance, int inode, const char *name) {
	struct directories_t *dirs = readdir_inode(instance, inode,name);
	struct directories_t *aux = dirs;
        aux->dir->name[aux->dir->name_len] = '\0';

	while (aux != NULL) {
		if (strcmp(name, aux->dir->name) == 0) {
			return aux->dir->inode;
		}
		aux = aux->next;
	}

	return -ENOTDIR;
}
struct ext2_inode* read_inode(ext2_fs_instance_t *instance, int inum) {

    int group; 
    unsigned int bnum; 
    struct ext2_inode *p; 
   int n; 
   __u64 inode_in_group, block_in_group, inode_in_block, inode_per_block,block_size ; 

   block_size = (1024 << instance->superblock.s_log_block_size);
   unsigned char *block = (unsigned char *)kmalloc(block_size,0);
   struct ext2_inode * buf_inode = (struct ext2_inode * )kmalloc(sizeof(struct ext2_inode),0);



 inode_per_block = block_size / instance->superblock.s_inode_size;

    group = (inum - 1)  / instance->superblock.s_inodes_per_group; 
    inode_in_group = (inum - 1) % instance->superblock.s_inodes_per_group; 
    block_in_group = inode_in_group / inode_per_block;
    inode_in_block = inode_in_group % inode_per_block; 

    bnum = instance->group_desc_table[group].bg_inode_table + block_in_group;  

         get_block (instance,bnum, block);

    p = (struct ext2_inode*) 
        (block + (inode_in_block * instance->superblock.s_inode_size )); 
        if (instance->superblock.s_inode_size  <= sizeof (struct ext2_inode))
                n = instance->superblock.s_inode_size;
        else
                n = sizeof (struct ext2_inode);

        memmove( (void*) buf_inode, (void *) p, n); 


    return  buf_inode ; 



}

static int write_inode(ext2_fs_instance_t *instance, int inum, struct ext2_inode* einode) {
	if (inum > 0) {

   int group; 
 __u64 inode_in_group, block_in_group, inode_in_block, inode_per_block,block_size ; 

    block_size = (1024 << instance->superblock.s_log_block_size);

    inode_per_block = block_size / instance->superblock.s_inode_size;

    group = (inum - 1)  / instance->superblock.s_inodes_per_group; 
    inode_in_group = (inum - 1) % instance->superblock.s_inodes_per_group; 
    block_in_group = inode_in_group / inode_per_block;
    inode_in_block = inode_in_group % inode_per_block; 
 
    __u32 bnum = instance->group_desc_table[group].bg_inode_table + block_in_group; 

    __u32 offset =  inode_in_block * instance->superblock.s_inode_size ;
		
		// update hardware
		instance->write_data(instance->super.device, einode, sizeof(struct ext2_inode), bnum * block_size + offset );
		
		return 0;
	}
	return -ENOENT;
}

static void set_block_inode_data(ext2_fs_instance_t *instance, struct ext2_inode *einode, int blk_n, __u32 blk) {
	if (blk_n < 12) { // Direct
		einode->i_block[blk_n] = blk;
		//write_inode(instance, einode->, einode); //XXX !!!!!
	} else if (blk_n < 12 + (1024 << instance->superblock.s_log_block_size) / 4) { // Indirect
		if (einode->i_block[12] == 0) {
			einode->i_block[12] = alloc_block(instance);
			//write_inode(instance, einode->, einode); //XXX !!!!!
		}
		int j = blk_n - 12; 
		__u32 addr_0 = einode->i_block[12]; 
		instance->write_data(instance->super.device, &blk, 4, addr_0 * (1024 << instance->superblock.s_log_block_size) + 4 * j);

	} else if (blk_n < 12 + (1024 << instance->superblock.s_log_block_size) / 4 + (1024 << instance->superblock.s_log_block_size) * (1024 << instance->superblock.s_log_block_size) / 16) { // Double indirect
		// TODO.
	} else {
		// TODO.
	}
}


static __u32 addr_inode_data(ext2_fs_instance_t *instance, int inode, int n_blk) {
	struct ext2_inode *einode = read_inode(instance, inode);
	if (einode) {
		return get_data_block(instance, einode, n_blk);
	}
	return 0;
}

static int getattr_inode(ext2_fs_instance_t *instance, int inode, struct stat *stbuf) {
	struct ext2_inode *einode = read_inode(instance, inode);
	if (einode) {
		stbuf->st_ino = inode + 1;
		stbuf->st_mode = einode->i_mode;
		stbuf->st_nlink = einode->i_links_count;
		stbuf->st_uid = einode->i_uid;
		stbuf->st_gid = einode->i_gid;
		stbuf->st_size = einode->i_size;
		stbuf->st_blksize = 1024 << instance->superblock.s_log_block_size;
		stbuf->st_blocks = einode->i_size / 512;
		stbuf->st_atime = einode->i_atime;
		stbuf->st_mtime = einode->i_mtime;
		stbuf->st_ctime = einode->i_ctime;
		return 0;
	}
	return -1;
}

static void setattr_inode(ext2_fs_instance_t *instance, int inode, struct stat *stbuf) {
	struct ext2_inode *einode = read_inode(instance, inode);
	if (einode) {
		einode->i_mode = stbuf->st_mode;
		einode->i_uid = stbuf->st_uid;
		einode->i_gid = stbuf->st_gid;
		einode->i_atime = stbuf->st_atime;
		einode->i_mtime = stbuf->st_mtime;
		einode->i_ctime = get_date();
		write_inode(instance, inode, einode);
	}
}

static __u32 alloc_block(ext2_fs_instance_t *instance) {
	int i;
	for (i = 0; i < instance->n_groups; i++) {
		if (instance->group_desc_table[i].bg_free_blocks_count) {
			// TODO: decrement bg_free_blocks_count
			__u32 addr_bitmap = instance->group_desc_table[i].bg_block_bitmap;
			__u8 *block_bitmap = (__u8*) kmalloc(1024 << instance->superblock.s_log_block_size,0);
			instance->read_data(instance->super.device, block_bitmap, (1024 << instance->superblock.s_log_block_size), addr_bitmap * (1024 << instance->superblock.s_log_block_size));

			__u32 ib;
			for (ib = 0; ib < instance->superblock.s_blocks_per_group; ib++) {
				if ((block_bitmap[ib/8] & (1 << (ib % 8))) == 0) {
					block_bitmap[ib/8] |= (1 << (ib % 8));
					instance->write_data(instance->super.device, &(block_bitmap[ib/8]), sizeof(__u8), addr_bitmap * (1024 << instance->superblock.s_log_block_size) + ib / 8);
					return ib + i * instance->superblock.s_blocks_per_group;
				}
			}
		}
	}
	return 0;
}

static int alloc_inode(ext2_fs_instance_t *instance, struct ext2_inode *inode) {
	int i;
	for (i = 0; i < instance->n_groups; i++) {
		if (instance->group_desc_table[i].bg_free_inodes_count) {
			__u8 *inode_bitmap = instance->group_desc_table_internal[i].inode_bitmap;

			// TODO: decrement bg_free_inodes_count
			__u32 addr_bitmap = instance->group_desc_table[i].bg_inode_bitmap;
			__u32 addr_table = instance->group_desc_table[i].bg_inode_table;

			__u32 ib;
			for (ib = 0; ib < instance->superblock.s_inodes_per_group; ib++) {
				int inode_n = i * instance->superblock.s_inodes_per_group + ib + 1;
				if ((inode_bitmap[ib/8] & (1 << (ib % 8))) == 0) {
					instance->write_data(instance->super.device, inode, sizeof(struct ext2_inode), addr_table * (1024 << instance->superblock.s_log_block_size) + sizeof(struct ext2_inode) * ib);
					inode_bitmap[ib/8] |= (1 << (ib % 8));
					instance->write_data(instance->super.device, &(inode_bitmap[ib/8]), sizeof(__u8), addr_bitmap * (1024 << instance->superblock.s_log_block_size) + ib / 8);
					return inode_n;
				}
			}
		}
	}
	return 0;
}

static int alloc_block_inode(ext2_fs_instance_t *instance, int inode) {
	struct ext2_inode *einode = read_inode(instance, inode);
	if (einode) {
		int i = 0;
		while (einode->i_block[i] > 0 && i < 12) i++;
		if (i < 12) {
			einode->i_block[i] = alloc_block(instance);
			write_inode(instance, inode, einode);
			return einode->i_block[i];
		}
	}
	return 0;
}

static void free_block(ext2_fs_instance_t *instance, __u32 blk) {
	int i = blk / instance->superblock.s_blocks_per_group;
	int ib = blk - instance->superblock.s_blocks_per_group * i; 
	__u32 addr_bitmap = instance->group_desc_table[i].bg_block_bitmap;

	__u8 block_bitmap;
	instance->read_data(instance->super.device, &(block_bitmap), sizeof(__u8), addr_bitmap * (1024 << instance->superblock.s_log_block_size) + ib / 8);
	block_bitmap &= ~(1 << (ib % 8));
	instance->write_data(instance->super.device, &(block_bitmap), sizeof(__u8), addr_bitmap * (1024 << instance->superblock.s_log_block_size) + ib / 8);
	
	// TODO: increment bg_free_blocks_count
}

#if 0
static void free_inode(ext2_fs_instance_t *instance, int inode) {
	int i = inode / instance->superblock.s_blocks_per_group; // Groupe de block.
	int ib = inode - instance->superblock.s_blocks_per_group * i; // Indice dans le groupe.
	int addr_bitmap = instance->group_desc_table[i].bg_inode_bitmap;

	__u8 block_bitmap;
	instance->read_data(instance->super.device, &(block_bitmap), sizeof(__u8), addr_bitmap * (1024 << instance->superblock.s_log_block_size) + ib / 8);
	block_bitmap &= ~(1 << (ib % 8));
	instance->write_data(instance->super.device, &(block_bitmap), sizeof(__u8), addr_bitmap * (1024 << instance->superblock.s_log_block_size) + ib / 8);
}
#endif

static void add_dir_entry(ext2_fs_instance_t *instance, int inode, const char *name, int type, int n_inode) {
	__u32 addr_debut = addr_inode_data(instance, inode, 0);

	if (addr_debut == 0) {
		
	}

	struct ext2_directory *dir = (struct ext2_directory*)kmalloc(sizeof(struct ext2_directory),0);
	instance->read_data(instance->super.device, dir, sizeof(struct ext2_directory), addr_debut);

	__u32 cur_pos = addr_debut;
	while (dir->rec_len + cur_pos < addr_debut + (1024 << instance->superblock.s_log_block_size)) {
		cur_pos += dir->rec_len;
		instance->read_data(instance->super.device, dir, sizeof(struct ext2_directory), cur_pos);
	}

	int s = 4 + 2 + 1 + 1 + dir->name_len;
	s += (4 - (s % 4)) % 4;
	dir->rec_len = s;
	instance->write_data(instance->super.device, dir, s, cur_pos);
	cur_pos += s;
	
	struct ext2_directory ndir;
	ndir.inode = n_inode;
	ndir.rec_len = addr_debut + (1024 << instance->superblock.s_log_block_size) - cur_pos;
	ndir.name_len = strlen(name);
	ndir.file_type = type;
	strcpy(ndir.name, name);
	int s2 = 4 + 2 + 1 + 1 + ndir.name_len;
	s2 += (4 - (s2 % 4)) % 4;
	instance->write_data(instance->super.device, &ndir, s2, cur_pos);
}

static void remove_dir_entry(ext2_fs_instance_t *instance, int inode, const char *name) {
	int addr_debut = addr_inode_data(instance, inode, 0);

	if (addr_debut == 0) {
		return;
	}

	struct ext2_directory dir, dir2;
	instance->read_data(instance->super.device, &dir, sizeof(struct ext2_directory), addr_debut);

	__u32 cur_pos = addr_debut;
	__u32 cur_pos2 = 0;
	int dec = 0;
	while (dir.rec_len + cur_pos < addr_debut + (1024 << instance->superblock.s_log_block_size)) {
		dir.name[dir.name_len] = '\0';
		if (strcmp(name, dir.name) == 0) {
			dec = dir.rec_len;
		}

		cur_pos2 = cur_pos;
		cur_pos += dir.rec_len;
		memcpy(&dir2, &dir, sizeof(dir));
		instance->read_data(instance->super.device, &dir, sizeof(struct ext2_directory), cur_pos);
		if (dec > 0) {
			if (dir.rec_len + cur_pos >= addr_debut + (1024 << instance->superblock.s_log_block_size)) {
				dir.rec_len += dec;
			}
			instance->write_data(instance->super.device, &dir, dir.rec_len, cur_pos - dec); //XXX
		}
	}

	if (dec == 0 && cur_pos2) {
		dir.name[dir.name_len] = '\0';
		if (strcmp(name, dir.name) == 0) {
			dir2.rec_len += dir.rec_len;
			instance->write_data(instance->super.device, &dir2, dir2.rec_len, cur_pos2); //XXX
		}
	}
}

static void init_dir(ext2_fs_instance_t *instance, int inode, int parent_inode) {
	__u32 addr = addr_inode_data(instance, inode, 0);

	if (addr == 0) {
		if (alloc_block_inode(instance, inode) >= 0) {
			addr = addr_inode_data(instance, inode, 0);
		} else {
			return;
		}
	}
	struct ext2_directory dir;
	dir.inode = inode;
  // inode (4) + rec_len (2) + name_len (1) + file_type (1) + name (1) + padding
	dir.rec_len = 4 + 2 + 1 + 1 + 4;
	dir.name_len = 1;
	dir.file_type = EXT2_FT_DIR;
	strcpy(dir.name, ".");

	instance->write_data(instance->super.device, &dir, sizeof(dir), addr);
	addr += dir.rec_len;

	dir.inode = parent_inode;
	dir.rec_len = (1024 << instance->superblock.s_log_block_size) - addr;
	dir.name_len = 2;
	dir.file_type = EXT2_FT_DIR;
	strcpy(dir.name, "..");
	instance->write_data(instance->super.device, &dir, sizeof(dir), addr);
}


static int mknod_inode(ext2_fs_instance_t *instance, int inode, const char *name, mode_t mode, dev_t dev __attribute__((unused))) {
	struct ext2_inode n_inode;
	memset(&n_inode, 0, sizeof(struct ext2_inode));
	n_inode.i_mode = mode;
	n_inode.i_links_count = 1;
// TODO uid/gid
//	n_inode.i_uid = fc->uid;
//	n_inode.i_gid = fc->gid;
	time_t cdate = get_date();
	n_inode.i_atime = cdate;
	n_inode.i_ctime = cdate;
	n_inode.i_mtime = cdate;
	
	int i = alloc_inode(instance, &n_inode);
	if (mode & EXT2_S_IFDIR) {
		add_dir_entry(instance, inode, name, EXT2_FT_DIR, i);
		init_dir(instance, i, inode);
	} else if (mode & EXT2_S_IFIFO) {
		add_dir_entry(instance, inode, name, EXT2_FT_FIFO, i);
	} else if (mode & EXT2_S_IFCHR) {
		add_dir_entry(instance, inode, name, EXT2_FT_CHRDEV, i);
	} else if (mode & EXT2_S_IFBLK) {
		add_dir_entry(instance, inode, name, EXT2_FT_BLKDEV, i);
	} else if (mode & EXT2_S_IFSOCK) {
		add_dir_entry(instance, inode, name, EXT2_FT_SOCK, i);
	} else if (mode & EXT2_S_IFLNK) {
		add_dir_entry(instance, inode, name, EXT2_FT_SYMLINK, i);
	} else {
		add_dir_entry(instance, inode, name, EXT2_FT_REG_FILE, i);
	}
	return i;
}

static void ext2inode_2_inode(inode_t* inode, struct _fs_instance_t *instance, int ino, struct ext2_inode *einode) {
	if (einode == NULL) {
		debug("inode is null! Can't convert it to real inode.");
		return;
	}
	inode->i_ino = ino;
	inode->i_mode = einode->i_mode;
	inode->i_uid = einode->i_uid;
	inode->i_gid = einode->i_gid;
	inode->i_size = einode->i_size;
	inode->i_atime = einode->i_atime;
	inode->i_ctime = einode->i_ctime;
	inode->i_dtime = einode->i_dtime;
	inode->i_mtime = einode->i_mtime;
	inode->i_nlink = einode->i_links_count;
	inode->i_blocks = einode->i_blocks;
	inode->i_instance = instance;
	inode->i_fops = &ext2fs_fops; 
	inode->i_fs_specific = NULL;
}

dentry_t * init_rootext2fs(ext2_fs_instance_t *instance) {
	dentry_t *root_ext2fs = (dentry_t *)kmalloc(sizeof(dentry_t),0);
	
	root_ext2fs->d_name = "";

	struct ext2_inode *einode = read_inode(instance, EXT2_ROOT_INO);
	root_ext2fs->d_inode = (inode_t *)kmalloc(sizeof(inode_t),0);
	ext2inode_2_inode(root_ext2fs->d_inode, (fs_instance_t*)instance, EXT2_ROOT_INO, einode);
	root_ext2fs->d_inode->i_count = 0;
	root_ext2fs->d_pdentry = NULL;

	return root_ext2fs;
}


