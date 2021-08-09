	.text

	.align 3
	
	.globl _strerror
	.private_extern _strerror
_strerror:
	.quad ___wine$func$msvcr70$717$strerror
