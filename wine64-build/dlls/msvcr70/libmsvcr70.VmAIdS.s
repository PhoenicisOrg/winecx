	.text

	.align 3
	
	.globl __isatty
	.private_extern __isatty
__isatty:
	.quad ___wine$func$msvcr70$278$_isatty
