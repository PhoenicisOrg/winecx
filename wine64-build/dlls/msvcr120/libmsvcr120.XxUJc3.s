	.text

	.align 3
	
	.globl _strcpy_s
	.private_extern _strcpy_s
_strcpy_s:
	.quad ___wine$func$msvcr120$1809$strcpy_s
