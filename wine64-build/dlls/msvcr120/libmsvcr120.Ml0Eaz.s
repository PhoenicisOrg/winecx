	.text

	.align 3
	
	.globl _malloc
	.private_extern _malloc
_malloc:
	.quad ___wine$func$msvcr120$1715$malloc
