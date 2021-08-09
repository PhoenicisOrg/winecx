	.text

	.align 3
	
	.globl _isprint
	.private_extern _isprint
_isprint:
	.quad ___wine$func$msvcr70$648$isprint
