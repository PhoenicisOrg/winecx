	.text

	.align 3
	
	.globl __wputenv
	.private_extern __wputenv
__wputenv:
	.quad ___wine$func$msvcr70$546$_wputenv
