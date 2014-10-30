/**
 * @file ext2_internal.h
 *
 * @section LICENSE
 *
 * Copyright (C) 2010, 2011, 2012 - TacOS developers.
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
 *
 * @section DESCRIPTION
 *
 * Description de ce que fait le fichier
 */

#ifndef _EXT2_INTERNAL_H_
#define _EXT2_INTERNAL_H_

#include <fs/devfs.h>
#include <vfs.h>
#include <kdirent.h>


#define EXT2_NDIR_BLOCKS                12
#define EXT2_IND_BLOCK                  EXT2_NDIR_BLOCKS
#define EXT2_DIND_BLOCK                 (EXT2_IND_BLOCK + 1)
#define EXT2_TIND_BLOCK                 (EXT2_DIND_BLOCK + 1)
#define EXT2_N_BLOCKS                   (EXT2_TIND_BLOCK + 1)
#define EXT2_ROOT_INO 2 



struct ext2_super_block {
 __le32    s_inodes_count;        /* Inodes count */
    __le32    s_blocks_count;        /* Blocks count */
    __le32    s_r_blocks_count;    /* Reserved blocks count */
    __le32    s_free_blocks_count;    /* Free blocks count */
    __le32    s_free_inodes_count;    /* Free inodes count */
    __le32    s_first_data_block;    /* First Data Block */
    __le32    s_log_block_size;    /* Block size */
    __le32    s_log_frag_size;    /* Fragment size */
    __le32    s_blocks_per_group;    /* # Blocks per group */
    __le32    s_frags_per_group;    /* # Fragments per group */
    __le32    s_inodes_per_group;    /* # Inodes per group */
    __le32    s_mtime;        /* Mount time */
    __le32    s_wtime;        /* Write time */
    __le16    s_mnt_count;        /* Mount count */
    __le16    s_max_mnt_count;    /* Maximal mount count */
    __le16    s_magic;        /* Magic signature */
    __le16    s_state;        /* File system state */
    __le16    s_errors;        /* Behaviour when detecting errors */
    __le16    s_minor_rev_level;     /* minor revision level */
    __le32    s_lastcheck;        /* time of last check */
    __le32    s_checkinterval;    /* max. time between checks */
    __le32    s_creator_os;        /* OS */
    __le32    s_rev_level;        /* Revision level */
    __le16    s_def_resuid;        /* Default uid for reserved blocks */
    __le16    s_def_resgid;        /* Default gid for reserved blocks */
    /*
     * These fields are for EXT2_DYNAMIC_REV superblocks only.
     *
     * Note: the difference between the compatible feature set and
     * the incompatible feature set is that if there is a bit set
     * in the incompatible feature set that the kernel doesn't
     * know about, it should refuse to mount the filesystem.
     * 
     * e2fsck's requirements are more strict; if it doesn't know
     * about a feature in either the compatible or incompatible
     * feature set, it must abort and not try to meddle with
     * things it doesn't understand...
     */
    __le32    s_first_ino;         /* First non-reserved inode */
    __le16   s_inode_size;         /* size of inode structure */
    __le16    s_block_group_nr;     /* block group # of this superblock */
    __le32    s_feature_compat;     /* compatible feature set */
    __le32    s_feature_incompat;     /* incompatible feature set */
    __le32    s_feature_ro_compat;     /* readonly-compatible feature set */
    __u8    s_uuid[16];        /* 128-bit uuid for volume */
    char    s_volume_name[16];     /* volume name */
    char    s_last_mounted[64];     /* directory where last mounted */
    __le32    s_algorithm_usage_bitmap; /* For compression */
    /*
     * Performance hints.  Directory preallocation should only
     * happen if the EXT2_COMPAT_PREALLOC flag is on.
     */
    __u8    s_prealloc_blocks;    /* Nr of blocks to try to preallocate*/
    __u8    s_prealloc_dir_blocks;    /* Nr to preallocate for dirs */
    __u16    s_padding1;
    /*
     * Journaling support valid if EXT3_FEATURE_COMPAT_HAS_JOURNAL set.
     */
    __u8    s_journal_uuid[16];    /* uuid of journal superblock */
    __u32    s_journal_inum;        /* inode number of journal file */
    __u32    s_journal_dev;        /* device number of journal file */
    __u32    s_last_orphan;        /* start of list of inodes to delete */
    __u32    s_hash_seed[4];        /* HTREE hash seed */
    __u8    s_def_hash_version;    /* Default hash version to use */
    __u8    s_reserved_char_pad;
    __u16    s_reserved_word_pad;
    __le32    s_default_mount_opts;
     __le32    s_first_meta_bg;     /* First metablock block group */
    __u32    s_reserved[190];    /* Padding to the end of the block */
};


#define EXT2_OS_LINUX		0 /* Linux */
#define EXT2_OS_HURD		1 /* Hurd */
#define EXT2_OS_MASIX		2 /* Masix */
#define EXT2_OS_FREEBSD		3 /* Freebsd */
#define EXT2_OS_LITES		4 /* Lites */

/*
 * Revision levels
 */
#define EXT2_GOOD_OLD_REV 0		/**< The good old (original) format */
#define EXT2_DYNAMIC_REV  1		/**< V2 format w/ dynamic inode sizes */

/**
 * Groupe.
 */
struct ext2_group_desc {
  __u32  bg_block_bitmap;		/**< Blocks bitmap block */
  __u32  bg_inode_bitmap;		/**< Inodes bitmap block */
  __u32  bg_inode_table;			/**< Inodes table block */
  __u16  bg_free_blocks_count;	/**< Free blocks count */
  __u16  bg_free_inodes_count;	/**< Free inodes count */
  __u16  bg_used_dirs_count;		/**< Directories count */
  __u16  bg_pad;					/**< unused. */
  __u32  bg_reserved[3];			/**< reserved. */
};


struct ext2_group_desc_internal {
	__u8 *inode_bitmap; /**< Inode bitmap. */	
};

/**
 * Structure of an inode on the disk
 */
struct ext2_inode {
	__u16	i_mode;		/**< File mode */
	__u16	i_uid;		/**< Low 16 bits of Owner Uid */
	__u32	i_size;		/**< Size in bytes */
	__u32	i_atime;	/**< Access time */
	__u32	i_ctime;	/**< Creation time */
	__u32	i_mtime;	/**< Modification time */
	__u32	i_dtime;	/**< Deletion Time */
	__u16	i_gid;		/**< Low 16 bits of Group Id */
	__u16	i_links_count;	/**< Links count */
	__u32	i_blocks;	/**< Blocks count */
	__u32	i_flags;	/**< File flags */
	__u32  i_osd1;		/**< OS dependent 1 */
	__u32	i_block[15];/**< Pointers to blocks */
	__u32	i_generation;	/**< File version (for NFS) */
	__u32	i_file_acl;	/**< File ACL */
	__u32	i_dir_acl;	/**< Directory ACL */
	__u32	i_faddr;	/**< Fragment address */
	__u32  i_osd2[3];	/**< OS dependent 2 */
};


#define EXT2_FT_UNKNOWN		0 
#define EXT2_FT_REG_FILE	1 
#define EXT2_FT_DIR		2 
#define EXT2_FT_CHRDEV		3 
#define EXT2_FT_BLKDEV		4 
#define EXT2_FT_FIFO		5 
#define EXT2_FT_SOCK		6 
#define EXT2_FT_SYMLINK		7 


struct ext2_directory {
	__u32	inode;      
	__u16	rec_len;    
	__u8		name_len;   
	__u8		file_type; 
	char		name[256];  
};


struct directories_t {
	struct ext2_directory *dir; 
	struct directories_t *next; 
};

// -- file format --
#define EXT2_S_IFSOCK	0xC000	/**< socket */
#define EXT2_S_IFLNK	0xA000	/**< symbolic link */
#define EXT2_S_IFREG	0x8000	/**< regular file */
#define EXT2_S_IFBLK	0x6000	/**< block device */
#define EXT2_S_IFDIR	0x4000	/**< directory */
#define EXT2_S_IFCHR	0x2000	/**< character device */
#define EXT2_S_IFIFO	0x1000	/**< fifo */
// -- process execution user/group override --
#define EXT2_S_ISUID	0x0800	/**< Set process User ID */
#define EXT2_S_ISGID	0x0400	/**< Set process Group ID */
#define EXT2_S_ISVTX	0x0200	/**< sticky bit */
// -- access rights --
#define EXT2_S_IRUSR	0x0100	/**< user read */
#define EXT2_S_IWUSR	0x0080	/**< user write */
#define EXT2_S_IXUSR	0x0040	/**< user execute */
#define EXT2_S_IRGRP	0x0020	/**< group read */
#define EXT2_S_IWGRP	0x0010	/**< group write */
#define EXT2_S_IXGRP	0x0008	/**< group execute */
#define EXT2_S_IROTH	0x0004	/**< others read */
#define EXT2_S_IWOTH	0x0002	/**< others write */
#define EXT2_S_IXOTH	0x0001	/**< others execute */

/**
 * @brief Instance de FS Ext2.
 */
typedef struct _ext2_fs_instance_t {
	fs_instance_t super; /**< Common fields for all FS instances. */
	dentry_t * root; /**< Inode root. */
	struct ext2_super_block superblock; /**< Superblock. */
	struct ext2_group_desc *group_desc_table; /**< Block group descriptor table. */
	int n_groups;   /**< Number of entries in the group desc table. */
	blkdev_read_t read_data; /**< Function to read data. */
	blkdev_write_t write_data; /**< Function to write data. */
	struct ext2_group_desc_internal *group_desc_table_internal; /**< Copy of inodes (only, for now?) */
} ext2_fs_instance_t;


void umount_EXT2(fs_instance_t *instance);


int ext2_read(open_file_descriptor * ofd, void * buf, size_t size);


int ext2_write (open_file_descriptor * ofd, const void *buf, size_t size);


int ext2_close(open_file_descriptor *ofd);


//int ext2_readdir(open_file_descriptor * ofd, char * entries, size_t size);


int ext2_stat(fs_instance_t *instance, const char *path, struct stat *stbuf);


int ext2_mknod(inode_t *dir, dentry_t *dentry, mode_t mode, dev_t dev);


int ext2_mkdir(inode_t *dir, dentry_t *dentry, mode_t mode);


int ext2_unlink(inode_t *dir, dentry_t *dentry);


int ext2_truncate(inode_t *inode, off_t off);


int ext2_rmdir(inode_t *dir, dentry_t *dentry);


int ext2_setattr(inode_t *inode, file_attributes_t *attr);


int ext2_seek(open_file_descriptor * ofd, long offset, int whence);


dentry_t *ext2_getroot(struct _fs_instance_t *instance);


dentry_t* ext2_lookup(struct _fs_instance_t *instance, struct _dentry_t* dentry, const char * name);


int ext2_rename(inode_t *old_dir, dentry_t *old_dentry, inode_t *new_dir, dentry_t *new_dentry);

dentry_t * init_rootext2fs(ext2_fs_instance_t *instance);

extern struct _open_file_operations_t ext2fs_fops;

#endif
