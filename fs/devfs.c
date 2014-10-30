
#include <fs/devfs.h>
#include <klibc.h>
#include <kmalloc.h>
#include <kerrno.h>
#include <kdirent.h>
#include <types.h>
#include <vfs.h>
#include <fs/fs.h>
#include <debug.h>

#define DEVFS_C  

#define MAX_DRIVERS 64 

static fs_instance_t* mount_devfs();
static void umount_devfs(fs_instance_t *instance);
static file_system_t dev_fs = {.name="DevFS", .unique_inode=0, .mount=mount_devfs, .umount=umount_devfs};

static int devfs_readdir(open_file_descriptor * ofd, char * entries, int size);

static dentry_t root_devfs;
static struct _open_file_operations_t devfs_fops = {.write = NULL, .read = NULL, .seek = NULL, .ioctl = NULL, .open = NULL, .close = NULL, .readdir = devfs_readdir};



typedef struct {
	char used; 
	char* name; 
	device_type_t type; 
        struct fs_dev_id_t *dev_id;
	void* di;
} driver_entry;

static driver_entry driver_list[MAX_DRIVERS];


void init_driver_list()
{
	int i;
	for(i=0; i<MAX_DRIVERS; i++)
	{
			driver_list[i].used = 0;
			driver_list[i].name = NULL;
			driver_list[i].type = CHARDEV;
			driver_list[i].di = NULL;
	}
}


driver_entry* find_driver(const char* name)
{
	int i = 0;
	driver_entry* ret = NULL;
	
	while(i<MAX_DRIVERS && ret == NULL)
	{
		if(driver_list[i].used)
		{
			if(strcmp(driver_list[i].name, name) == 0)
				ret = &driver_list[i];
		}
		i++;
	}
	
	return ret;
}


int devfs_register_chardev(const char* name, chardev_interfaces* di, struct fs_dev_id_t *dev_id)
{
	int i = 0;
	int done = 0;
	while(i<MAX_DRIVERS && !done)
	{
		if(!driver_list[i].used)
		{
			driver_list[i].used = 1;
			driver_list[i].name = (char*)name;
			driver_list[i].di = di;
                        driver_list[i].dev_id = dev_id;
			driver_list[i].type = CHARDEV;
			done = 1;
		}
		
		i++;
	}
	return done-1; 
}



int devfs_register_blkdev( const char* name, blkdev_interfaces* di, struct fs_dev_id_t *dev_id)
{
	int i = 0;
	int done = 0;
      
	while(i<MAX_DRIVERS && !done)
	{
		if(!driver_list[i].used)
		{
			driver_list[i].used = 1;
			driver_list[i].name = (char*)name;
                        driver_list[i].di = di;
			driver_list[i].dev_id = dev_id;
			driver_list[i].type = BLKDEV;
			done = 1;
		}
		
		i++;
	}
	return done-1; 
}

static int devfs_readdir(open_file_descriptor * ofd, char * entries, int size) {
	int count = 0;

	int i = ofd->current_octet;
	while (i < MAX_DRIVERS && count < size) {
		if (driver_list[i].used) {
			struct dirent *d = (struct dirent *)(entries + count);
			int reclen = sizeof(d->d_ino) + sizeof(d->d_reclen) + sizeof(d->d_type) + strlen(driver_list[i].name) + 1;
			if (count + reclen > size) break;
			d->d_reclen = reclen;
			d->d_ino = 1;
			//d.d_type = dir_entry->attributes;
			strcpy(d->d_name, driver_list[i].name);
			count += d->d_reclen;
		}
		i++;
	}
	ofd->current_octet = i;

	return count;
}

static dentry_t *devfs_getroot() {
	return &root_devfs;
}

static dentry_t* devfs_lookup(struct _fs_instance_t *instance, struct _dentry_t* dentry, const char * name) {
	
    

	driver_entry *drentry = find_driver(name);


   
	if (drentry != NULL) {

  inode_t *inode = kmalloc(sizeof(inode_t),0);
		inode->i_ino = 42; //XXX
		inode->i_mode = 0755;
		inode->i_uid = 0;
		inode->i_gid = 0;
		inode->i_size = 0;
		inode->i_atime = inode->i_ctime = inode->i_mtime = inode->i_dtime = 0; // XXX
		inode->i_nlink = 1;
		inode->i_blocks = 1;
		inode->i_count = 0;
		inode->i_instance = instance;
		inode->i_fops = kmalloc(sizeof(open_file_operations_t),0);
		inode->i_fs_specific = (blkdev_interfaces*)(drentry->di);
                inode->dev_id = (struct fs_dev_id_t*)(drentry->dev_id);
		switch(drentry->type) {
			case CHARDEV:
				inode->i_fops->readdir = NULL;
				inode->i_fops->open = ((chardev_interfaces*)(drentry->di))->open;
				inode->i_fops->write = ((chardev_interfaces*)(drentry->di))->write;
				inode->i_fops->read = ((chardev_interfaces*)(drentry->di))->read; 
				inode->i_fops->seek = NULL; 
				inode->i_fops->close = ((chardev_interfaces*)(drentry->di))->close;
				inode->i_fops->ioctl = ((chardev_interfaces*)(drentry->di))->ioctl;
				
				break;
			case BLKDEV:
				inode->i_fops->readdir = NULL;
				inode->i_fops->open = ((blkdev_interfaces*)(drentry->di))->open;
				inode->i_fops->write = NULL; 
				inode->i_fops->read = NULL; 
				inode->i_fops->seek = NULL; 
				inode->i_fops->close = ((blkdev_interfaces*)(drentry->di))->close;
				inode->i_fops->ioctl = ((blkdev_interfaces*)(drentry->di))->ioctl;
				
				break;
		}
		dentry_t *d = kmalloc(sizeof(dentry_t),0);
		char *n = strdup(name);
		d->d_name = (const char*)n;
		d->d_inode = inode;
		d->d_pdentry = dentry;
		return d;
	}
	return NULL;
}

static fs_instance_t* mount_devfs() {
//	debug("mounting DevFS");

	fs_instance_t *instance = kmalloc(sizeof(fs_instance_t),0);
	memset(instance, 0, sizeof(fs_instance_t));
	instance->fs = &dev_fs;
//	instance->stat = devfs_stat;
	instance->getroot = devfs_getroot;
	instance->lookup = devfs_lookup;
	
	return instance;
}

static void umount_devfs(fs_instance_t *instance) {
	kfree(instance);
}

void devfs_init() {
	root_devfs.d_name = "dev";
	root_devfs.d_inode = kmalloc(sizeof(inode_t),0);
	memset(root_devfs.d_inode, 0, sizeof(inode_t));
	root_devfs.d_inode->i_count = 0;
	root_devfs.d_inode->i_ino = 0;
	root_devfs.d_inode->i_mode = S_IFDIR | 00755;
	root_devfs.d_inode->i_fops = &devfs_fops;
	root_devfs.d_pdentry = NULL;
	init_driver_list();
	vfs_register_fs(&dev_fs);
}
