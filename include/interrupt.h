
#ifndef _INTERRUPT_H_
#define _INTERRUPT_H_

#define save_flags(flags) \
  asm volatile("pushfl ; popl %0":"=g"(flags)::"memory")
#define restore_flags(flags) \
  asm volatile("push %0; popfl"::"g"(flags):"memory")


#define disable_IRQs(flags)    \
  ({ save_flags(flags); asm("cli\n"); })
#define restore_IRQs(flags)    \
  restore_flags(flags)

#endif
