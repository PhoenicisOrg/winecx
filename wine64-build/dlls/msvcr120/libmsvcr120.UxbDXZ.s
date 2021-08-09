	.text

	.align 3
	
	.globl _isprint
	.private_extern _isprint
_isprint:
	.quad ___wine$func$msvcr120$1661$isprint
