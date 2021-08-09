	.text

	.align 3
	
	.globl _ftell
	.private_extern _ftell
_ftell:
	.quad ___wine$func$msvcr120$1633$ftell
