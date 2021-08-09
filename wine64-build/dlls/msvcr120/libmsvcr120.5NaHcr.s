	.text

	.align 3
	
	.globl _memcpy_s
	.private_extern _memcpy_s
_memcpy_s:
	.quad ___wine$func$msvcr120$1727$memcpy_s
