	.text

	.align 3
	
	.globl _fflush
	.private_extern _fflush
_fflush:
	.quad ___wine$func$msvcr120$1596$fflush
