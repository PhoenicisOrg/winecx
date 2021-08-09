	.text

	.align 3
	
	.globl _strerror
	.private_extern _strerror
_strerror:
	.quad ___wine$func$msvcr120$1811$strerror
