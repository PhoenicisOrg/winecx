	.text

	.align 3
	
	.globl __getcwd
	.private_extern __getcwd
__getcwd:
	.quad ___wine$func$msvcr120$710$_getcwd
