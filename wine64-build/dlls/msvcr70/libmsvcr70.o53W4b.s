	.text

	.align 3
	
	.globl __purecall
	.private_extern __purecall
__purecall:
	.quad ___wine$func$msvcr70$401$_purecall
