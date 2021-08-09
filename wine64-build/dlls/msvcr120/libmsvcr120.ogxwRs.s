	.text

	.align 3
	
	.globl __wputenv
	.private_extern __wputenv
__wputenv:
	.quad ___wine$func$msvcr120$1396$_wputenv
