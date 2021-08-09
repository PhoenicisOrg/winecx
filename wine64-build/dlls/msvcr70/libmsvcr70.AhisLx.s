	.text

	.align 3
	
	.globl _modf
	.private_extern _modf
_modf:
	.quad ___wine$func$msvcr70$684$modf
