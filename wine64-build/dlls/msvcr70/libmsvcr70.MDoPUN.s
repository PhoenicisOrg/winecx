	.text

	.align 3
	
	.globl _swprintf
	.private_extern _swprintf
_swprintf:
	.quad ___wine$func$msvcr70$732$swprintf
