	.text

	.align 3
	
	.globl __putenv
	.private_extern __putenv
__putenv:
	.quad ___wine$func$msvcr120$1050$_putenv