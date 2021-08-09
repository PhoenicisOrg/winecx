	.text

	.align 3
	
	.globl _memset
	.private_extern _memset
_memset:
	.quad ___wine$func$msvcr120$1730$memset
