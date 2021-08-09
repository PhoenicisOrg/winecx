	.text

	.align 3
	
	.globl _strerror_s
	.private_extern _strerror_s
_strerror_s:
	.quad ___wine$func$msvcr120$1812$strerror_s
