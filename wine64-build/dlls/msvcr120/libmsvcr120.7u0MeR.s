	.text

	.align 3
	
	.globl _strncmp
	.private_extern _strncmp
_strncmp:
	.quad ___wine$func$msvcr120$1817$strncmp
