
#include "crt.h"
#include "libc.h"


/**
 * Max number of environment variables
 */
#define MAX_ENVVARS 1024

char **environ = NULL;


void exit ()
{
  _exit();
}

unsigned int strlen(register const char *str)
{
  unsigned int retval = 0;
  
  while (*str++)
    retval++;
  
  return retval;
}

int mount(const char *source, const char *target,
	  const char *filesystemtype, unsigned long mountflags,
	  const char *data)
{
  return _mount(source, target, filesystemtype, mountflags, data);
}


static void * kernel_heap_top = NULL;
int brk(void *end_data_seg)
{
  if (! end_data_seg)
    return -1;

  kernel_heap_top = _brk(end_data_seg);
  return 0;
}

/**
 * The presence of this global variable without any protected access
 * to it explains why the "malloc/calloc" functions below are
 * MT-unsafe !
 */
static void * malloc_heap_top = NULL;
void * malloc (size_t size)
{
  void * retval;

  if (size <= 0)
    return NULL;

  /* Align on a 4B boundary */
  size = ((size-1) & ~3) + 4;

  if (! kernel_heap_top)
    kernel_heap_top = _brk(0);

  if (! malloc_heap_top)
    malloc_heap_top = kernel_heap_top;

  retval = malloc_heap_top;
  malloc_heap_top += size;

  _brk(malloc_heap_top);
  return retval;
}


void free(void *ptr)
{
  //  Free ignored (not implemented yet)
}




int strcmp(const char *dst,const char *src)
{
	int i = 0;

	while ((dst[i] == src[i])) {
		if (src[i++] == 0)
			return 0;
	}

	return 1;
}



