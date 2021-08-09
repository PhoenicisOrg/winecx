	.text

	.align 3
	
	.globl _strcmp
	.private_extern _strcmp
_strcmp:
	.quad ___wine$func$msvcr120$1806$strcmp
