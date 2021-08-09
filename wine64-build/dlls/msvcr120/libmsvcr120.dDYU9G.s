	.text

	.align 3
	
	.globl __errno
	.private_extern __errno
__errno:
	.quad ___wine$func$msvcr120$597$_errno
