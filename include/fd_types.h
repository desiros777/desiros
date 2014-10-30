

#ifndef _FD_TYPES_H
#define _FD_TYPES_H

#include <types.h>

#define FOPEN_MAX 500 

#define SEEK_SET 0 
#define SEEK_CUR 1 
#define SEEK_END 2 

struct _open_file_descriptor;
struct _fs_instance_t;
struct _dentry_t;


struct _open_file_operations_t {

	int (*write)(struct _open_file_descriptor *, const void*, size_t);

	int (*read)(struct _open_file_descriptor *,void*, size_t);

	int (*seek)(struct _open_file_descriptor *, long, int);

	int (*ioctl)(struct _open_file_descriptor*, unsigned int, void *);

	int (*open) (struct _open_file_descriptor*);

	int (*close) (struct _open_file_descriptor*);

	int (*readdir) (struct _open_file_descriptor*, char*, int);

} open_file_operations_t;


typedef struct _open_file_descriptor {
	__u32 flags;
	int current_cluster;
	char * pathname;
	__u32 current_octet; 
	__u8 select_sem;
	struct _fs_instance_t *fs_instance;
	struct _inode_t *inode;
	struct _dentry_t *dentry;
	struct _mounted_fs_t *mnt;
	struct _open_file_operations_t *f_ops;
	void * i_fs_specific;
	void * extra_data; 
} open_file_descriptor;

#endif
