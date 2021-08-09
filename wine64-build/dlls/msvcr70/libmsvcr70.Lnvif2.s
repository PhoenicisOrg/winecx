	.text

	.align 3
	
	.globl __putch
	.private_extern __putch
__putch:
	.quad ___wine$func$msvcr70$402$_putch
