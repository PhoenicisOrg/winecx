	.text

	.align 3
	
	.globl _isspace
	.private_extern _isspace
_isspace:
	.quad ___wine$func$msvcr120$1663$isspace
