	.text

	.align 3
	
	.globl _vwprintf
	.private_extern _vwprintf
_vwprintf:
	.quad ___wine$func$msvcr70$751$vwprintf
