	.text

	.align 3
	
	.globl __set_errno
	.private_extern __set_errno
__set_errno:
	.quad ___wine$func$msvcr120$1086$_set_errno
