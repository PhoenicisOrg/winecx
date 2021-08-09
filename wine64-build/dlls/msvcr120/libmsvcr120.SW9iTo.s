	.text

	.align 3
	
	.globl _realloc
	.private_extern _realloc
_realloc:
	.quad ___wine$func$msvcr120$1763$realloc
