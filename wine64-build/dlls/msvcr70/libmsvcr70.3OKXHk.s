	.text

	.align 3
	
	.globl __chdir
	.private_extern __chdir
__chdir:
	.quad ___wine$func$msvcr70$171$_chdir
