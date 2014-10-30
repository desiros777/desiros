
#ifndef _VFS_H_
#define _VFS_H_

#include <types.h>
#include <kstat.h>
#include <fd_types.h>
#include <fs/fs.h>
struct _fs_instance_t;
struct _dentry_t;
struct _inode_t;
struct _file_attributes_t;


typedef struct {
	char *name;	
	int unique_inode; 
	struct _fs_instance_t * (*mount) (open_file_descriptor*); 
	void (*umount) (struct _fs_instance_t *); 
} file_system_t;


typedef struct _fs_instance_t {
	file_system_t *fs;							
	open_file_descriptor * device;								
	struct _dentry_t* (*getroot) (struct _fs_instance_t *); 
	struct _dentry_t* (*lookup) (struct _fs_instance_t *, struct _dentry_t*, const char *); 
	int (*mkdir) (struct _inode_t *, struct _dentry_t *, mode_t);											
	int (*mknod) (struct _inode_t *, struct _dentry_t *, mode_t, dev_t);								
	int (*stat) (struct _inode_t *, struct stat *);									
	int (*unlink) (struct _inode_t *, struct _dentry_t *);															
	int (*rmdir) (struct _inode_t *, struct _dentry_t *);																
	int (*truncate) (struct _inode_t *, off_t size);	
	int (*setattr) (struct _inode_t *inode, struct _file_attributes_t *attr);
	int (*rename) (struct _inode_t *old_dir, struct _dentry_t *old_dentry, struct _inode_t *new_dir, struct _dentry_t *new_dentry); 
} fs_instance_t;


typedef struct _mounted_fs_t {
	fs_instance_t *instance; 
	char *name; 
	struct _mounted_fs_t *next; 
} mounted_fs_t;

/**
 * Structure inode.
 */
typedef struct _inode_t {
	unsigned long i_ino; /**< Inode number */
	int  i_mode;   /**< File mode */
	uid_t  i_uid;    /**< Low 16 bits of Owner Uid */
	gid_t  i_gid;    /**< Low 16 bits of Group Id */
	off_t  i_size;   /**< Size in bytes */
	time_t  i_atime;  /**< Access time */
	time_t  i_ctime;  /**< Creation time */
	time_t  i_mtime;  /**< Modification time */
	time_t  i_dtime;  /**< Deletion Time */
	int i_count; 
	unsigned char  i_nlink;  /**< Links count */
	unsigned char  i_flags;  /**< File flags */
	unsigned long  i_blocks; /**< Blocks count */
        struct fs_dev_id_t *dev_id;
	fs_instance_t *i_instance; 
	struct _open_file_operations_t *i_fops; /**< Ofd operations. */
 
	void *i_fs_specific; /**< Extra data */
} inode_t;

/**
 * Directory Entry.
 */
typedef struct _dentry_t {
	const char *d_name; 
	inode_t *d_inode; 
	struct _dentry_t *d_pdentry; 
} dentry_t;


struct nameidata {
	int flags; 
	dentry_t *dentry; 
	mounted_fs_t *mnt; 
	const char *last;
};

#define ATTR_UID 1 
#define ATTR_GID (1 << 1) 
#define ATTR_MODE (1 << 2) 
#define ATTR_ATIME (1 << 3) 
#define ATTR_MTIME (1 << 4) 
#define ATTR_CTIME (1 << 5) 
#define ATTR_SIZE (1 << 6) 

typedef struct _file_attributes_t {
	int mask; 
	struct stat stbuf; 
        int ia_size;
} file_attributes_t;


void vfs_register_fs(file_system_t *fs);


open_file_descriptor* vfs_open(const char * pathname, __u32 flags);


void vfs_mount(const char *device, const char *mountpoint, const char *type);


int vfs_umount(const char *mountpoint);


int vfs_mkdir(const char * pathname, mode_t mode);


int vfs_stat(const char *pathname, struct stat * stbuf);


int vfs_unlink(const char *pathname);


int vfs_mknod(const char * path, mode_t mode, dev_t dev);


int vfs_chmod(const char *pathname, mode_t mode);


int vfs_chown(const char *pathname, uid_t owner, gid_t group);


int vfs_utimes(const char *pathname, const struct timeval tv[2]);


int vfs_rename(const char *oldpath, const char *newpath);


int vfs_rmdir(const char *pathname);


int vfs_readdir(open_file_descriptor * ofd, char * entries, size_t size);


int vfs_close(open_file_descriptor *ofd);

void vfs_init();

#endif
