
#include <fd_types.h>
#include <debug.h>
#include <process.h>
#include <vfs.h>

int sys_write( __u32 fd, const void *buf, __u32 c) {
	struct process     *process = current ;
	int ret ;
	
	open_file_descriptor *ofd;

	if (process->fd[fd]) {
		ofd = process->fd[fd];
		
		if(ofd->f_ops->read == NULL) {
			debug("No \"write\" method for this device.");
			ret = -1;
		} else {
			ret = ofd->f_ops->write(ofd, (void*) buf, c);
		}
	} else {
		ret = -1;
	}

         return ret;
}

int sys_open( char *path , __u32 flags) {
	int i=0;
	int fd_id;
	struct process     *process = current ;
	
	// recherche d une place dans la file descriptor table du process
	while (process->fd[i]) { 
		i++;
	}

           
	// ouverture du fichier
	if ((process->fd[i] = vfs_open(path, flags)) != NULL) {
		fd_id = i;
	} else {
		fd_id = -1;
	}

        return fd_id;

}

int sys_read( __u32 fd,void *buf, __u32 c) {

	struct process     *process = current ;
	int ret ;
	
	open_file_descriptor *ofd;



	if (process->fd[fd]) {
		ofd = process->fd[fd];
		
		if(ofd->f_ops->read == NULL) {
			debug("No \"read\" method for this device.");
			ret = -1;
		} else {

			ret = ofd->f_ops->read(ofd, (void*) buf, c);
		}
	} else {
		ret = -1;
	}

         return ret;
}

int sys_seek( __u32 fd, long *offset, int whence) {
	struct process     *process = current ;
	int ret ;
	
	open_file_descriptor *ofd;

	if (process->fd[fd]) 
		ofd = process->fd[fd];
		
		if(ofd->f_ops->seek == NULL) {
			debug("No \"seek\" method for this device.");
			*offset = -1;
		} else {
			ofd->f_ops->seek(ofd, *offset, whence);
			*offset = ofd->current_octet;
		}

}

int sys_stat(const char *path, struct stat *buf, int *ret) {
	*ret = vfs_stat(path, buf);
}



