	.text

	.align 3
	
	.globl _ferror
	.private_extern _ferror
_ferror:
	.quad ___wine$func$msvcr120$1590$ferror
