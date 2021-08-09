	.text

	.align 3
	
	.globl __setjmp
	.private_extern __setjmp
__setjmp:
	.quad ___wine$func$msvcr70$422$_setjmp
