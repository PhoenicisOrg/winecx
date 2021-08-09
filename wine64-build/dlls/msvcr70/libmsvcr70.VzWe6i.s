	.text

	.align 3
	
	.globl __getdrive
	.private_extern __getdrive
__getdrive:
	.quad ___wine$func$msvcr70$256$_getdrive
