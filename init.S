# Copyright (C) 2004,2005  The DESIROS Team
#      desiros.dev@gmail.com
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307,
# USA.


.global _asm_default_int,_asm_irq_0, _asm_exc_PF,_asm_syscalls,_go

.macro	SAVE_REGS 

  
  pushal
  pushl %ds
  pushl %es
  pushl %fs
  pushl %gs
  
 
  movw $0x10,%bx
  movw %bx,%ds
  movw %bx,%es
  movw %bx,%fs
  movw %bx,%gs


.endm

.macro	RESTORE_REGS 

  /* Restore the user context */
  
  popl %gs
  popl %fs
  popl %es
  popl %ds
  popal
  

 
.endm



_asm_default_int:
	SAVE_REGS
	call isr_default_int
	movb $0x20,%al
	outb %al,$0x20
	RESTORE_REGS
	iret

_asm_irq_0:
	SAVE_REGS
	call isr_clock_int
	movb $0x20,%al
	outb %al,$0x20
	RESTORE_REGS
	iret

_asm_exc_PF:
	SAVE_REGS
	call isr_page_fault
	RESTORE_REGS
        add  $4,%esp 
	iret

_asm_syscalls:
	SAVE_REGS
	pushl %esp /* user_ctxt */
	pushl %eax
	call do_syscalls
	addl $8, %esp
        /* store the do_syscall return value into interrupted context */
        movl %eax,44(%esp)
	RESTORE_REGS
	iret


