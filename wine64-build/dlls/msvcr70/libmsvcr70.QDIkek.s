	.text

	.align 3
	
	.globl _ftell
	.private_extern _ftell
_ftell:
	.quad ___wine$func$msvcr70$629$ftell
