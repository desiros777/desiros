#include <io.h>
#include <klibc.h>
#include <cpu_context.h>
#include <kmalloc.h>
#include <mm.h>
#include <process.h>
#include <debug.h>
#include <list.h>
#include <syscall.h>
#include <kerrno.h>


int do_syscalls(int syscall_id,const struct cpu_state *user_ctxt)
{


     int ret ;

       
  switch(syscall_id)
    {


        case SYSCALL_ID_EXEC:{
                   
           __u32 user_str, len , argc ;
          __u32  len_args;
           char **src_argaddr;
           char **ap;
           char **param;
           char * str;
           int i;
          /* Get the user arguments */
	ret = syscall_get4args(user_ctxt, & user_str, & len,
				      & src_argaddr, & len_args);

            if (ret != 0 )
	          break;
        ap = src_argaddr;
	argc = 0;
 
         while (*ap++) 
		argc++;

         if (argc) {
		param = (char**) kmalloc(sizeof(char*) * (argc+1),0);
		for (i=0 ; i<argc ; i++) {
			param[i] = (char*) kmalloc(strlen(src_argaddr[i]) + 1, 0);
			strcpy(param[i], src_argaddr[i]);
		}
		param[i] = 0;
	}


          str =(char*) kmalloc(len +1, 0);

          if (! str)
	  {
	    ret = -3;
	    break;
	  }
   
         ret = strzcpy_from_user(str,user_str,len + 1);

          if (0 > ret)
	  {
	    kfree((__u32)str);
	    break;
	  } 

           sys_exec(str, param );

         struct process     *proc = current ;
         asm("mov %0, %%eax; mov %%eax, %%cr3"::"m"(proc->regs.cr3));
             
      }

      break;
    
         case 2:{
           __u32 u_str, len ;
           
           
           char * str;
               
          ret = syscall_get2args(user_ctxt, & u_str, & len);

            if (ret != 0 )
	          break;
          

          str =(char*) kmalloc(len +1, 0);

          if (! str)
	  {
	    ret = -3;
	    break;
	  }
   
         ret = strzcpy_from_user(str,u_str,len + 1);

          if (0 > ret)
	  {
	    kfree((__u32)str);
	    break;
	  } 


          kprintf("%s",(char*)str);

 
      
     kfree((__u32)str); 

          
              
      }
      break;


case 3:{
       
       sys_exit();
              
      }
      break;
 
case SYSCALL_ID_MOUNT :{
        
         __u32 user_src;
	size_t srclen;
	char * kernel_src = NULL;
	__u32 user_target;
	size_t target_len;
	char * kernel_target;
	__u32 mountflags;
	__u32 user_fstype;
	char * kernel_fstype;
	__u32 user_args;
	char * kernel_args = NULL;


      
         ret = syscall_get7args( user_ctxt ,&user_src ,&srclen,
				      &user_target , &target_len , &user_fstype,
				      &mountflags,&user_args);

          
          if (user_src != (__u32)NULL)
	  {
            kernel_src =(char*) kmalloc(srclen +1 , 0);
            ret = strzcpy_from_user(kernel_src,user_src,srclen + 1);
	 }

            kernel_target =(char*) kmalloc(target_len + 1 , 0);
            ret = strzcpy_from_user(kernel_target,user_target,target_len  + 1);

	if ( OK != ret)
	  {
	    if (kernel_src)
	      kfree((__u32)kernel_src);
	    break;
	  }

          kernel_fstype =(char*) kmalloc(256 , 0);
          ret = strzcpy_from_user(kernel_fstype,user_fstype,256);

	if (OK != ret)
	  {
	    if (kernel_src)
	    kfree((__u32)kernel_src);
	    kfree((__u32)kernel_target);
	    break;
	  }

	if (user_args != (__u32)NULL)
	  {
          kernel_args =(char*) kmalloc(1024 , 0);
          ret = strzcpy_from_user(kernel_args,user_args,1024);

	    if (OK != ret)
	      {
		if (kernel_src)
		kfree((__u32)kernel_src);
		kfree((__u32)kernel_target);
		kfree((__u32)kernel_fstype);
		break;
	      }
	  }

          vfs_mount(kernel_src, kernel_target, kernel_fstype );
  
              
      }
      break;

case SYSCALL_ID_OPEN :{
        
        __u32 user_str;
	__u32  len;
	__u32  open_flags;
	char * path;

      ret = syscall_get3args(user_ctxt,
				  &user_str, &len,&open_flags);

           path =(char*) kmalloc(len +1 , 0);
           ret = strzcpy_from_user(path,user_str,len + 1);

       if (OK != ret)
	break;

        ret = sys_open( path , open_flags);
          kfree((__u32)path);
             
      }
      break;

case SYSCALL_ID_WRITE :{
        
        __u32 uaddr_buf;
	__u32 buflen;

	int fd;
            ret = syscall_get3args(user_ctxt,
				     &fd,&uaddr_buf,&buflen);
        if (OK != ret)
	  break;

         ret=sys_write(fd,(void*)uaddr_buf,buflen);

              
      }
      break;


case SYSCALL_ID_READ:{
        __u32 uaddr_buf;
	__u32 buflen;

	int fd;
            ret = syscall_get3args(user_ctxt,
				     &fd,&uaddr_buf,&buflen);
        if (OK != ret)
	  break;

         ret=sys_read(fd,(void*)uaddr_buf,buflen);
              
      }
      break;


case SYSCALL_ID_BRK:
      {
	__u32 new_top_heap;
	struct uvmm_as * as;
        struct process     *process = current ;

	as = process_get_address_space(process);
        if (! as)
         {
          debug();
          return -7;
         }

	ret = syscall_get1arg(user_ctxt,&new_top_heap);
	if (OK != ret)
	  break;

	ret = uvmm_brk(as, new_top_heap);

      }
      break;


      default:
      kprintf("unknown syscall %d\n", syscall_id);
      break;
    }
  
 
  return 0 ;

	
}




