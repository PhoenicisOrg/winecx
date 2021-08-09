	.text

	.align 3
	
	.globl _memcmp
	.private_extern _memcmp
_memcmp:
	.quad ___wine$func$msvcr70$679$memcmp
