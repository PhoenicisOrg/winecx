	.text

	.align 3
	
	.globl _printf
	.private_extern _printf
_printf:
	.quad ___wine$func$msvcr120$1751$printf
