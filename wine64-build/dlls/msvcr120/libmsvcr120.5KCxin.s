	.text

	.align 3
	
	.globl _memcmp
	.private_extern _memcmp
_memcmp:
	.quad ___wine$func$msvcr120$1725$memcmp
