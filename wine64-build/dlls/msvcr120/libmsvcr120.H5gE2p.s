	.text

	.align 3
	
	.globl __realloc_crt
	.private_extern __realloc_crt
__realloc_crt:
	.quad ___wine$func$msvcr120$1058$_realloc_crt
