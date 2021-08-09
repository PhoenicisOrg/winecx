	.text

	.align 3
	
	.globl _modf
	.private_extern _modf
_modf:
	.quad ___wine$func$msvcr120$1731$modf
