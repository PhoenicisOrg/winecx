	.text

	.align 3
	
	.globl __errno
	.private_extern __errno
__errno:
	.quad ___wine$func$msvcr70$202$_errno
