	.text

	.align 3
	
	.globl __putch
	.private_extern __putch
__putch:
	.quad ___wine$func$msvcr120$1048$_putch
