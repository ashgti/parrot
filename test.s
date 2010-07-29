	.section	__TEXT,__text,regular,pure_instructions
	.globl	_main
	.align	4, 0x90
_main:                                  ## @main
## BB#0:
	subq	$8, %rsp
	xorl	%edi, %edi
	callq	_Parrot_new
	movq	%rax, (%rsp)
	leaq	(%rsp), %rsi
	movq	%rax, %rdi
	callq	_Parrot_init_stack
	leaq	_.LC0(%rip), %rdi
	callq	_puts
	xorl	%eax, %eax
	addq	$8, %rsp
	ret

	.section	__TEXT,__cstring,cstring_literals
_.LC0:                                  ## @.LC0
	.asciz	 "Hello World\n"


.subsections_via_symbols
