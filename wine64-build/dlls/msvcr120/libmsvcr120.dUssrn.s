	.text

	.align 3
	
	.globl _strcat
	.private_extern _strcat
_strcat:
	.quad ___wine$func$msvcr120$1803$strcat
