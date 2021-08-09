	.text

	.align 3
	
	.globl __popen
	.private_extern __popen
__popen:
	.quad ___wine$func$msvcr120$1041$_popen
