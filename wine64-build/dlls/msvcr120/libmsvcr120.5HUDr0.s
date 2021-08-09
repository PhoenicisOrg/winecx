	.text

	.align 3
	
	.globl _vprintf
	.private_extern _vprintf
_vprintf:
	.quad ___wine$func$msvcr120$1870$vprintf
