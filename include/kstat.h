

#ifndef _KSTAT_H_
#define _KSTAT_H_

#include <types.h>
#include <time.h>

/* File types.  */
#define	S_IFMT	 0170000 /**< These bits determine file type.  */
#define S_IFSOCK 0140000 /**< Socket.  */
#define S_IFLNK  0120000 /**< Symbolic link.  */
#define S_IFREG  0100000 /**< Regular file.  */
#define S_IFBLK  0060000 /**< Block device.  */
#define S_IFDIR  0040000 /**< Directory.  */
#define S_IFCHR  0020000 /**< Character device.  */
#define S_IFIFO  0010000 /**< FIFO.  */

#define S_IXOTH 001   /**< Execute by other. */
#define S_IROTH 002   /**< Read by other. */
#define S_IWOTH 004   /**< Write by other. */
#define S_IXGRP 0010  /**< Execute by group. */
#define S_IRGRP 0020  /**< Read by group. */
#define S_IWGRP 0040  /**< Write by group. */
#define S_IXUSR 00100 /**< Execute by user. */
#define S_IRUSR 00200 /**< Read by user. */
#define S_IWUSR 00400 /**< Write by user. */

#define S_ISBLK(m)  (((m) & S_IFMT) == S_IFBLK) /**< Is block device special file. */
#define S_ISCHR(m)  (((m) & S_IFMT) == S_IFCHR) /**< Is char device special file. */
#define S_ISDIR(m)  (((m) & S_IFMT) == S_IFDIR) /**< Is a directory. */
#define S_ISFIFO(m) (((m) & S_IFMT) == S_IFIFO) /**< Is a FIFO. */
#define S_ISLNK(m)  (((m) & S_IFMT) == S_IFLNK) /**< Is a link. */
#define S_ISREG(m)  (((m) & S_IFMT) == S_IFREG) /**< Is a regular file. */
#define S_ISSOCK(m) (((m) & S_IFMT) == S_IFSOCK) /**< Is a socket. */

typedef __u32 mode_t; 
typedef __u32 dev_t; 
typedef __u32 uid_t; 
typedef __u32 gid_t; 
typedef __u32 nlink_t;
typedef __u32 blksize_t;
typedef __u32 blkcnt_t;
typedef __u32 ino_t; 


struct stat {
	dev_t     st_dev;      
	ino_t     st_ino;     
	mode_t    st_mode;    
	nlink_t   st_nlink;    
	uid_t     st_uid;      
	gid_t     st_gid;      
	dev_t     st_rdev;     
	off_t     st_size;     
	blksize_t st_blksize;  
	blkcnt_t  st_blocks;   
	time_t    st_atime;    
	time_t    st_mtime;    
	time_t    st_ctime;   
};


#endif
