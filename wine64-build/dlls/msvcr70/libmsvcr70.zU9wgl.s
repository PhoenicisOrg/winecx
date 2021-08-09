	.text

	.align 3
	
	.globl __popen
	.private_extern __popen
__popen:
	.quad ___wine$func$msvcr70$400$_popen
