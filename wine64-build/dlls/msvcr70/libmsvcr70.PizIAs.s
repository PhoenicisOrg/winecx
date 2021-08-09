	.text

	.align 3
	
	.globl __wgetenv
	.private_extern __wgetenv
__wgetenv:
	.quad ___wine$func$msvcr70$535$_wgetenv
