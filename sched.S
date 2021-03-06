.global do_switch

do_switch:
	// retrieve the address *current 
	mov (%esp),%esi
	pop %eax			// pops @current

       // prepare les registres
	push  4(%esi)	// eax
	push  8(%esi)	// ecx
	push  12(%esi)	// edx
	push  16(%esi)	// ebx
	push  24(%esi)	// ebp
	push  28(%esi)	// esi
	push  32(%esi)	// edi
	push  48(%esi)	// ds
	push  50(%esi)	// es
	push  52(%esi)	// fs
	push  54(%esi)	// gs

	// removed the mask of PIC
	mov $0x20,%al
	out %al, $0x20

	// load page table
	mov 56(%esi),%eax
	mov %eax,%cr3

	// load registers
	pop %gs
	pop %fs
	pop %es
	pop %ds
	pop %edi
	pop %esi
	pop %ebp
	pop %ebx
	pop %edx
	pop %ecx
	pop %eax

	iret

