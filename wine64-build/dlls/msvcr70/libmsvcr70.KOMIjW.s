	.text

	.align 3
	
	.globl __wchdir
	.private_extern __wchdir
__wchdir:
	.quad ___wine$func$msvcr70$493$_wchdir
