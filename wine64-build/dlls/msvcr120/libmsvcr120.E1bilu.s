	.text

	.align 3
	
	.globl _fopen
	.private_extern _fopen
_fopen:
	.quad ___wine$func$msvcr120$1615$fopen
