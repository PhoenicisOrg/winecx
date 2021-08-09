	.text

	.align 3
	
	.globl _wscanf
	.private_extern _wscanf
_wscanf:
	.quad ___wine$func$msvcr70$775$wscanf
