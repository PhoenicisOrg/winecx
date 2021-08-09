	.text

	.align 3
	
	.globl __getws
	.private_extern __getws
__getws:
	.quad ___wine$func$msvcr70$265$_getws
