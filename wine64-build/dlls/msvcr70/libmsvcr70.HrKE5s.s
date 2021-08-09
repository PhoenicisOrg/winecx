	.text

	.align 3
	
	.globl _srand
	.private_extern _srand
_srand:
	.quad ___wine$func$msvcr70$709$srand
