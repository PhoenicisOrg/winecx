	.text

	.align 3
	
	.globl _getc
	.private_extern _getc
_getc:
	.quad ___wine$func$msvcr70$633$getc
