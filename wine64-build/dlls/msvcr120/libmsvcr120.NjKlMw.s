	.text

	.align 3
	
	.globl __strerror
	.private_extern __strerror
__strerror:
	.quad ___wine$func$msvcr120$1147$_strerror
