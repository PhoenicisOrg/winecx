	.text

	.align 3
	
	.globl _abort
	.private_extern _abort
_abort:
	.quad ___wine$func$msvcr70$581$abort
