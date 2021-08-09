	.text

	.align 3
	
	.globl _memcpy
	.private_extern _memcpy
_memcpy:
	.quad ___wine$func$msvcr70$680$memcpy
