	.text

	.align 3
	
	.globl __strerror
	.private_extern __strerror
__strerror:
	.quad ___wine$func$msvcr70$449$_strerror
