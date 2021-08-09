	.text

	.align 3
	
	.globl __isatty
	.private_extern __isatty
__isatty:
	.quad ___wine$func$msvcr120$755$_isatty
