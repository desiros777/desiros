
#include <types.h>
#include <mm.h>
#include <debug.h>
#include <kerrno.h>
#include <kmalloc.h>

int usercpy(__u32 dst_uaddr,__u32 src_uaddr, size_t size)
{
  __u32 kern_addr;
  int retval;

  if (size <= 0)
    return 0;

  /* Make sure user is trying to access user space */
  if (! PAGING_IS_USER_AREA(src_uaddr, size) )
    return -EPERM;
  if (! PAGING_IS_USER_AREA(dst_uaddr, size) )
    return -EPERM;

  kern_addr = kmalloc(size, 0);
  if (! kern_addr)
    return -ENOMEM;

  memcpy(kern_addr, src_uaddr,size);
  memcpy(dst_uaddr, kern_addr,size);

  kfree((__u32)kern_addr);

  return size;
}



static int nocheck_user_strzcpy(char *dst, const char *src,
				      __u32 len)
{

  unsigned int i;
 


  for (i = 0; i < len; i++)
    {
      dst[i] = src[i];
      if(src[i] == '\0')
        break;
    }
  
  dst[len-1] = '\0'; 


  return 0;

}


int strzcpy_from_user(char *kernel_to, __u32 user_from,
				__u32 max_len)
{

  /* Don't allow invalid max_len */
  if ( (max_len < 1) || (max_len > PAGING_USER_SPACE_SIZE) ){
     debug();
    return -1;
   }

  return nocheck_user_strzcpy(kernel_to, (const char*)user_from, max_len);
}
