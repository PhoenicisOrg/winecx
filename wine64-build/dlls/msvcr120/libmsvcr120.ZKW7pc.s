	.text

	.align 3
	
	.globl _ungetc
	.private_extern _ungetc
_ungetc:
	.quad ___wine$func$msvcr120$1860$ungetc
