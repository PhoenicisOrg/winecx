	.text

	.align 3
	
	.globl _nanf
	.private_extern _nanf
_nanf:
	.quad ___wine$func$msvcr120$1734$nanf
