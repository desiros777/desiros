#ifndef _USER_CRT_H_
#define _USER_CRT_H_

#include <types.h>

int _console_write(char * str);

void _exit () __attribute__((noreturn));

int _mount(const char *source, const char *target,
	       const char *filesystemtype, unsigned long mountflags,
	       const char *data);

int _open(const char * pathname, int flags);

int _read(int fd, char * buf, __u32  len);

int _write(int fd, const char * buf, __u32 len);

/**
 * Syscall to re-initialize the address space of the current process
 * with that of the program 'progname'
 *
 * The args argument is the address of an array that contains the
 * arguments and environnement variables for the new process.
 */
int _exec(const char * prog,
	      void const* args,
	      size_t arglen);

/**
 * Syscall to get/set heap top address
 */
void * _brk(void * new_top_address);
#endif

