	.text

	.align 3
	
	.globl __malloc_crt
	.private_extern __malloc_crt
__malloc_crt:
	.quad ___wine$func$msvcr120$885$_malloc_crt
