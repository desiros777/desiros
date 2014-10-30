C=gcc
AR=ar
CP=cp
STRIP=strip
OBJCOPY=objcopy
CFLAGS  = -Wall -nostdinc -ffreestanding -I. -I..
LIBGCC  = $(shell $(CC) -print-libgcc-file-name) # To benefit from FP/64bits artihm.
LDFLAGS = -Wl,--warn-common -nostdlib -Wl,-Tldscript.lds

# Main target
all: userprogs.kimg

-include .mkvars

PROGS := myprog3 myprog2

# Build dependencies of the programs
$(PROGS) : % : %.o crt.o libc.a
PWD := $(shell pwd)

# Programs generation
$(PROGS):
	$(CC) -static $(LDFLAGS) -o $@ $^ $(LIBGCC)

# Generation of the libC
libc.a: libc.o 

# Create a program image to be integrated into the Kernel
userprogs.kimg: $(PROGS)
	@echo "# Generating ELF images for inclusion into the kernel image: $@"
	@echo "SECTIONS { .userprogs . : { " > .userprogs.lds
	@i=0 ;                                                              \
	 for f in $^ ; do                                                   \
           i=`expr $$i + 1` ;                                               \
	   echo "extern char _begin_userprog$$i, _end_userprog$$i;"         \
                > .userprog$$i.c ;                                          \
	   echo "char *_userprog"$$i"_entry[]" >> .userprog$$i.c ;          \
	   echo "  __attribute__((section(\".userprogs_table\")))"          \
                >> .userprog$$i.c ;                                         \
	   echo "  = { \"$$f\", &_begin_userprog$$i, &_end_userprog$$i };"  \
                >> .userprog$$i.c ;                                         \
	   $(CC) $(CFLAGS) -c .userprog$$i.c -o .userprog$$i.o ;            \
           $(CP) $$f $$f.strip && $(STRIP) -sx $$f.strip ;                  \
           $(OBJCOPY) --add-section .userprog$$i=$$f.strip .userprog$$i.o   \
                .userprog$$i.kimg ;                                         \
           echo "  . = ALIGN(4096);" >> .userprogs.lds ;                    \
           echo "  _begin_userprog$$i = .;" >> .userprogs.lds ;             \
	   echo "  .userprog$$i.kimg(.userprog$$i);" >> .userprogs.lds ;    \
	   echo "  _end_userprog$$i = .;" >> .userprogs.lds ;               \
	   echo "  .userprog$$i.kimg(.rodata);.userprog$$i.kimg(.data);"   \
                >> .userprogs.lds ;                                         \
         done
	@echo "  _userprogs_table = .; *(.userprogs_table) ; LONG(0);"      \
                >> .userprogs.lds
	@echo "} /DISCARD/ : { *(.bss) } }"               \
                >> .userprogs.lds
	@$(LD) -r -o $@ -T.userprogs.lds

# Create libraries from object files
%.a:
	$(AR) rcv $@ $^

# Create objects from C source code
%.o: %.c
	$(CC) "-I../include" -c $< $(CFLAGS) -o $@

# Create objects from assembler (.S) source code
%.o: %.S
	$(CC) -I$(PWD) -c $< $(CFLAGS) -DASM_SOURCE=1 -o $@

# Clean directory
clean:
	$(RM) *.o *.a *~ $(PROGS) *.kimg *.strip
	$(RM) .userprog*
