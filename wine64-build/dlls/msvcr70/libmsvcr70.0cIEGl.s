	.text

	.align 3
	
	.globl __aligned_malloc
	.private_extern __aligned_malloc
__aligned_malloc:
	.quad ___wine$func$msvcr70$153$_aligned_malloc
