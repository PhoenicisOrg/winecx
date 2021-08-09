	.text

	.align 3
	
	.globl _atol
	.private_extern _atol
_atol:
	.quad ___wine$func$msvcr120$1471$atol
