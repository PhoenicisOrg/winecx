	.text

	.align 3
	
	.globl _raise
	.private_extern _raise
_raise:
	.quad ___wine$func$msvcr70$694$raise
