	.text

	.align 3
	
	.globl _lrint
	.private_extern _lrint
_lrint:
	.quad ___wine$func$msvcr120$1709$lrint
