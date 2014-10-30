
#include <types.h>
#include <kerrno.h>

struct cpu_state {

  /* Segment registers */
  __u32 gs;
  __u32 fs;
  __u32 es;
  __u32 ds;

/* General registers, accessed with pushal/popal */
  __u32 edi;
  __u32 esi; 
  __u32 ebp; 
  __u32 esp;   /* points just below eax */
  __u32 ebx;  
  __u32 edx; 
  __u32 ecx; 
  __u32 eax;

  /* Flags */
  __u32 eflags;
  /* Code segment:offset */
  __u32 eip;
  __u32 cs;
  /* Optional stack contents */
  __u32 return_addr;
  __u32 param[0];

} __attribute__((packed));


inline int syscall_get3args(const struct cpu_state *user_ctxt,
			       /* out */unsigned int *arg1,
			       /* out */unsigned int *arg2,
			       /* out */unsigned int *arg3)
{
  *arg1 = user_ctxt->ebx;
  *arg2 = user_ctxt->ecx;
  *arg3 = user_ctxt->edx; 
  return 0 ;
}

int syscall_get1arg(const struct  cpu_state *user_ctxt,
			      /* out */unsigned int *arg1)
{
  unsigned int unused;
  return syscall_get3args(user_ctxt, arg1, & unused, & unused);
}


int syscall_get2args(const struct cpu_state *user_ctxt,
			       /* out */unsigned int *arg1,
			       /* out */unsigned int *arg2)
{
  unsigned int unused;
  return syscall_get3args(user_ctxt, arg1, arg2, & unused);
}

int syscall_get4args(const struct cpu_state *user_ctxt,
			       /* out */unsigned int *arg1,
			       /* out */unsigned int *arg2,
			       /* out */unsigned int *arg3,
			       /* out */unsigned int *arg4)
{
  __u32 uaddr_other_args;
  __u32 other_args[2];
  int    retval;

  /* Retrieve the 3 arguments. The last one is an array containing the
     remaining arguments */
  retval = syscall_get3args(user_ctxt, arg1, arg2,
				(unsigned int *)& uaddr_other_args);
  if (OK != retval)
    return retval;
  
   /* Copy the array containing the remaining arguments from user
     space */
  memcpy((unsigned int)other_args,
				(unsigned int)uaddr_other_args,
				sizeof(other_args));

  *arg3 = other_args[0];
  *arg4 = other_args[1];
  return OK;
}


int syscall_get7args(const struct cpu_state *user_ctxt,
			       /* out */unsigned int *arg1,
			       /* out */unsigned int *arg2,
			       /* out */unsigned int *arg3,
			       /* out */unsigned int *arg4,
			       /* out */unsigned int *arg5,
			       /* out */unsigned int *arg6,
			       /* out */unsigned int *arg7)
{
  __u32  uaddr_other_args;
  unsigned int other_args[5];
  int    retval;

  /* Retrieve the 3 arguments. The last one is an array containing the
     remaining arguments */
  retval = syscall_get3args(user_ctxt, arg1, arg2,
				(unsigned int *)& uaddr_other_args);
  if (OK != retval)
    return retval;
  
  /* Copy the array containing the remaining arguments from user
     space */
  memcpy((unsigned int)other_args,
				(unsigned int)uaddr_other_args,
				sizeof(other_args));


  *arg3 = other_args[0];
  *arg4 = other_args[1];
  *arg5 = other_args[2];
  *arg6 = other_args[3];
  *arg7 = other_args[4];
  return OK;
}

