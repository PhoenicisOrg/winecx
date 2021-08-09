	.text

	.align 3
	
	.globl _difftime
	.private_extern _difftime
_difftime:
	.quad ___wine$func$msvcr70$600$difftime
