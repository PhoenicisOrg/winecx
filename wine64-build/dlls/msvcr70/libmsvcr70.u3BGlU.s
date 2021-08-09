	.text

	.align 3
	
	.globl _fwprintf
	.private_extern _fwprintf
_fwprintf:
	.quad ___wine$func$msvcr70$630$fwprintf
