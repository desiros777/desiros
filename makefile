## Copyright (C) 2004,2005  The DESIROS Team
##       desiros.dev@gmail.com
## This program is free software; you can redistribute it and/or
## modify it under the terms of the GNU General Public License
## as published by the Free Software Foundation; either version 2
## of the License, or (at your option) any later version.
## 
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
## 
## You should have received a copy of the GNU General Public License
## along with this program; if not, write to the Free Software
## Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307,
## USA. 

CC=gcc
LD=ld
CP=cp
CFLAGS  = -g -Wall -fno-builtin
LIBGCC  = $(shell $(CC) -print-libgcc-file-name) # To benefit from FP/64bits artihm.
LDFLAGS = -nostdlib 

FS_OBJ = vfs.o fs/devfs.o fs/ext2/ext2.o fs/ext2/ext2_functions.o 
         
MEM_OBJ =  mem/physmem.o mem/paging.o mem/kvmm_slab.o mem/kvmm.o mem/kmalloc.o mem/uvmm.o 

DRIVER_OBJ = drivers/pci.o drivers/zero.o drivers/console.o drivers/ide.o drivers/partition.o 

OBJECTS = multiboot.o gdt.o klibc.o init.o interrupt.o idt.o pic.o cpu_context.o uacess.o syscalls.o \
	clock.o process.o sched.o schedule.o  elf32.o syscall/exit.o syscall/exec.o  syscall/kunistd.o \
        $(MEM_OBJ) $(DRIVER_OBJ) $(FS_OBJ) ksynch.o kwaitq.o block_dev.o kernel.o userland/userprogs.kimg 
       				

KERNEL_OBJ   = desiros_core

# Main target
all: $(KERNEL_OBJ)

$(KERNEL_OBJ): $(OBJECTS) ./desiros.lds
	$(LD) $(LDFLAGS) -T ./desiros.lds -o $@ $(OBJECTS) $(LIBGCC)
	-nm -C $@ | cut -d ' ' -f 1,3 > desiros.map
	size $@

# Create the userland programs to include in the kernel image
userland/userprogs.kimg: FORCE
	$(MAKE) -C userland

FORCE:
	@

# Create objects from C source code
%.o: %.c
	$(CC) "-I$(PWD)/include" -c "$<" $(CFLAGS) -o "$@"
	

# Create objects from assembler (.S) source code
%.o: %.S
	$(CC) "-I$(PWD)/include" -c "$<" $(CFLAGS) -DASM_SOURCE=1 -o "$@"

clean:
	$(RM) *.o  *.elf *.bin *.map desiros_core
	$(RM) syscall/*.o syscall/*~
	$(RM) drivers/*.o drivers/*~
	$(RM) mem/*.o mem/*~
	$(RM) fs/*.o fs/*~
	$(RM) fs/ext2/*.o fs/ext2/*~
	@cd $(PWD)/userland && $(MAKE) $@

          
