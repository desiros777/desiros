#ifndef __SYSCALL_H
#define __SYSCALL_H

#include <types.h>

#define SYSCALL_ID_EXEC         258 

/*
 * File system interface
 */
#define  SYSCALL_ID_MOUNT       555
#define  SYSCALL_ID_OPEN        559 
#define  SYSCALL_ID_READ        561 
#define  SYSCALL_ID_WRITE       563 
#define  SYSCALL_ID_BRK         303

int sys_open( char *path , __u32 flags);
int sys_read( __u32 fd,void *buf, __u32 c);
void sys_exec(char * str, void const* argv );
#endif
