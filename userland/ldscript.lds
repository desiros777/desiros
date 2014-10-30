
/* We generate binary in the ELF format */
OUTPUT_FORMAT("elf32-i386","elf32-i386","elf32-i386");

/* The entry point of the program is _start (defined in crt.c) */
ENTRY(_start)

/* The architecture is i386 */
OUTPUT_ARCH("i386")

SECTIONS
{
    /* our program is loaded at 2G */
    . = 0x40000000;

    /* Beginning of the text section */
    .text :
    {   
	/* This section includes the code */
        *(.text*)
	/* Defines the 'etext' and '_etext' at the end */
        PROVIDE(etext = .);
        PROVIDE(_etext = .);
    }

    /* Beginning of the data section */
    . = ALIGN(4096);
    .data . :
    {   *(.data*) 
        PROVIDE(edata = .);
	PROVIDE(_edata = .);
    }

    /* Beginning of the read-only data section */
    .rodata . :
    {   *(.rodata*)
	*(.eh_frame*)
        PROVIDE(erodata = .);
        PROVIDE(_erodata = .);
    }

    /* Beginning of the BSS section (global uninitialized data) */
    .bss SIZEOF(.rodata) + ADDR(.rodata) :
    {   PROVIDE(bbss = .);
	PROVIDE(_bbss = .);
	*(.bss)
        *(COMMON)
        PROVIDE(ebss = .);
	PROVIDE(_ebss = .);
    }
}


