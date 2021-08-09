	.text

	.align 3
	
	.globl _calloc
	.private_extern _calloc
_calloc:
	.quad ___wine$func$msvcr120$1485$calloc
