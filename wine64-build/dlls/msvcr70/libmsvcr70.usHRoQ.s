	.text

	.align 3
	
	.globl _scanf
	.private_extern _scanf
_scanf:
	.quad ___wine$func$msvcr70$700$scanf
