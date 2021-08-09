	.text

	.align 3
	
	.globl _strstr
	.private_extern _strstr
_strstr:
	.quad ___wine$func$msvcr70$726$strstr
