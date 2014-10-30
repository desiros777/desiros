


/* The entry point is _start  */
ENTRY(_start)


SECTIONS
{
    /* our kernel is loaded at 0x100000 */
    . = 0x100000;
    __b_load   = .;


    /* Defines a symbol '__b_kernel to mark the start of the kernel
       code/data */
    . = ALIGN(4096);
    __b_kernel = .;
  
    /* Beginning of the text section */
    .text ALIGN(4096): 
    {   
	/* This section includes the code */
        *(.text*)
	/* Defines the 'etext' and '_etext' at the end */
        etext = .;
        _etext = .;
    }

    /* Beginning of the data section */
    .data . :
    {   *(.data*) 
        edata = .;
	_edata = .;
    }

    /* Beginning of the read-only data section */
    .rodata . :
    {   *(.rodata*)
	*(.eh_frame*)

	/* For articles 7.5 and later, it is better if the program
           "files" are located on a 4kB boundary: this allows
           binfmt_elf32 to share program pages between kernel and
           user, alleviating the need to allocate new pages to copy the
           user code */
	. = ALIGN(4096);
	*(.userprogs)

        erodata = .;
        _erodata = .;
    }

    /* We take note of the end of the data to load */
    __e_load = .;

    /* Beginning of the BSS section (global uninitialized data) */
    .bss SIZEOF(.rodata) + ADDR(.rodata) :
    {   *(.bss*)
        *(COMMON*)

	/* We put the stack of the bootstrap thread on a page
           boundary, because it can be un-allocated later */
	. = ALIGN(4096);
	*(.init_stack)

        ebss = .;
       _ebss = .;
    }

    /* We take note of the end of the kernel: this is where the GPFM
       will begin */
    __e_kernel = .;
}
