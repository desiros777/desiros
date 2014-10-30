
#include <libc.h>
#include <syscall.h>

inline
int _syscall3(int id,
		  unsigned int arg1,
		  unsigned int arg2,
		  unsigned int arg3)
{
   int ret;

  asm volatile("movl %1,%%eax \n"
	       "movl %2,%%ebx \n"
	       "movl %3,%%ecx \n"
	       "movl %4,%%edx \n"
	       "int  %5\n"
	       "movl %%eax, %0"
	       :"=g"(ret)
	       :"g"(id),"g"(arg1),"g"(arg2),"g"(arg3)
	        ,"i"(0x80)
	       :"eax","ebx","ecx","edx","memory");

  return ret;
}

int _syscall0(int id)
{
  return _syscall3(id, 0, 0, 0);
}

int _syscall1(int id,
	     unsigned int arg1)
{
  return _syscall3(id, arg1, 0, 0);
}


int _syscall2(int id,
		  unsigned int arg1,
		  unsigned int arg2)
{
  return _syscall3(id, arg1, arg2, 0);
}

int _syscall4(int id,
		  unsigned int arg1,
		  unsigned int arg2,
		  unsigned int arg3,
		  unsigned int arg4)
{
  unsigned int args[] = { arg3, arg4 };
  return _syscall3(id, arg1, arg2, (unsigned)args);
}


int _syscall7(int id,
		  unsigned int arg1,
		  unsigned int arg2,
		  unsigned int arg3,
		  unsigned int arg4,
		  unsigned int arg5,
		  unsigned int arg6,
		  unsigned int arg7)
{
  unsigned int args[] = { arg3, arg4, arg5, arg6, arg7 };
  return _syscall3(id, arg1, arg2, (unsigned)args);
}


void _exit()
{
  _syscall0(0x03);
  
  /* Never reached ! */
  for ( ; ; )
    ;
}

int _console_write(char * str)
{
  return _syscall2(0x02, str,
		       strlen(str));
}

int _mount(const char *source, const char *target,
	       const char *filesystemtype, unsigned long mountflags,
	       const char *args)
{
  if (!target || !filesystemtype)
    return -1;

  return  _syscall7( SYSCALL_ID_MOUNT,
		       (unsigned int)source, source?strlen(source):0,
		       (unsigned int)target, strlen(target),
		       (unsigned int)filesystemtype,
		       mountflags,
		       (unsigned int)args);
}


int _open(const char * pathname, int flags)
{
  if (! pathname)
    return -1;

  return _syscall3(SYSCALL_ID_OPEN,
		       (unsigned)pathname,
		       strlen(pathname),
		       flags);
}

int _read(int fd, char * buf, __u32  len)
{
  return _syscall3(SYSCALL_ID_READ, fd,
		       (unsigned int) buf,
		       (unsigned int) len);
}

int _write(int fd, const char * buf, __u32 len)
{
  return _syscall3(SYSCALL_ID_WRITE, fd,
		       (unsigned int) buf,
		       (unsigned int) len);
}

void * _brk(void * new_top_address)
{
  return (void*)_syscall1(SYSCALL_ID_BRK,
			      (unsigned)new_top_address);
}

int _exec(const char * prog,
	      void const* args,
	      size_t arglen)
{
  return _syscall4(SYSCALL_ID_EXEC, (unsigned int)prog,
		       (unsigned int)strlen(prog),
		       (unsigned int)args,
		       (unsigned int)arglen);
}


