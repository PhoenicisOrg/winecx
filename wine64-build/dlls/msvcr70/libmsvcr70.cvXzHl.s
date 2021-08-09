	.text

	.align 3
	
	.globl __memicmp
	.private_extern __memicmp
__memicmp:
	.quad ___wine$func$msvcr70$385$_memicmp
