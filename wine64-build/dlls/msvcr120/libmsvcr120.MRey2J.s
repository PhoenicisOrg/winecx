	.text

	.align 3
	
	.globl _atof
	.private_extern _atof
_atof:
	.quad ___wine$func$msvcr120$1469$atof
