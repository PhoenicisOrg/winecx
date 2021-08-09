	.text

	.align 3
	
	.globl __getdiskfree
	.private_extern __getdiskfree
__getdiskfree:
	.quad ___wine$func$msvcr70$254$_getdiskfree
