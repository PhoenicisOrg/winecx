	.text

	.align 3
	
	.globl _isspace
	.private_extern _isspace
_isspace:
	.quad ___wine$func$msvcr70$650$isspace
