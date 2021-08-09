	.text

	.align 3
	
	.globl __wgetcwd
	.private_extern __wgetcwd
__wgetcwd:
	.quad ___wine$func$msvcr70$533$_wgetcwd
