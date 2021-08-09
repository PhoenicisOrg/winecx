	.text

	.align 3
	
	.globl _strcoll
	.private_extern _strcoll
_strcoll:
	.quad ___wine$func$msvcr120$1807$strcoll
