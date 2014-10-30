
#ifndef _CPU_CONTEXT_H_
#define _CPU_CONTEXT_H_

/**
 * Opaque structure storing the CPU context of an inactive kernel or
 * user thread, as saved by the low level primitives below or by the
 * interrupt/exception handlers.
 */
struct cpu_state;


inline int syscall_get3args(const struct cpu_state *user_ctxt,
			       unsigned int *arg1,
			       unsigned int *arg2,
			       unsigned int *arg3);

int syscall_get1arg(const struct  cpu_state *user_ctxt,
			      unsigned int *arg1);

int syscall_get2args(const struct cpu_state *user_ctxt,
			       unsigned int *arg1,
			       unsigned int *arg2);

#endif


