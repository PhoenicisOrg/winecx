	.text

	.align 3
	
	.globl _abort
	.private_extern _abort
_abort:
	.quad ___wine$func$msvcr120$1447$abort
