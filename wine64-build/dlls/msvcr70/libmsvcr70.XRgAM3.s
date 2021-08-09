	.text

	.align 3
	
	.globl _longjmp
	.private_extern _longjmp
_longjmp:
	.quad ___wine$func$msvcr70$673$longjmp
