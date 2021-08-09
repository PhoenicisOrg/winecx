	.text

	.align 3
	
	.globl _getchar
	.private_extern _getchar
_getchar:
	.quad ___wine$func$msvcr70$634$getchar
