	.text

	.align 3
	
	.globl _longjmp
	.private_extern _longjmp
_longjmp:
	.quad ___wine$func$msvcr120$1708$longjmp
