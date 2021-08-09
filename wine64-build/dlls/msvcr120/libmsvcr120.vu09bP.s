	.text

	.align 3
	
	.globl __getpid
	.private_extern __getpid
__getpid:
	.quad ___wine$func$msvcr120$718$_getpid
