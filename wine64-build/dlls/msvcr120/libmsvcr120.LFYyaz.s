	.text

	.align 3
	
	.globl __get_errno
	.private_extern __get_errno
__get_errno:
	.quad ___wine$func$msvcr120$691$_get_errno
