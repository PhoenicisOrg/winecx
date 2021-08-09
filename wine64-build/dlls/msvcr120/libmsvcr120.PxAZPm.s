	.text

	.align 3
	
	.globl __chdir
	.private_extern __chdir
__chdir:
	.quad ___wine$func$msvcr120$532$_chdir
