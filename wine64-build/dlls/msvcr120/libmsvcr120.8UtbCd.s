	.text

	.align 3
	
	.globl __memicmp
	.private_extern __memicmp
__memicmp:
	.quad ___wine$func$msvcr120$1022$_memicmp
