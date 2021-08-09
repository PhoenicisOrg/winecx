	.text

	.align 3
	
	.globl _getenv
	.private_extern _getenv
_getenv:
	.quad ___wine$func$msvcr120$1641$getenv
