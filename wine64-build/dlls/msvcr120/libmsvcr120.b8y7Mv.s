	.text

	.align 3
	
	.globl _getenv_s
	.private_extern _getenv_s
_getenv_s:
	.quad ___wine$func$msvcr120$1642$getenv_s
