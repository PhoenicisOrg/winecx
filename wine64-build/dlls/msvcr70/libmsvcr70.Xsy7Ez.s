	.text

	.align 3
	
	.globl __getpid
	.private_extern __getpid
__getpid:
	.quad ___wine$func$msvcr70$260$_getpid
