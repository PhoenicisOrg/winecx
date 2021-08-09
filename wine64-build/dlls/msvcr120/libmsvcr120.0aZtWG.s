	.text

	.align 3
	
	.globl _puts
	.private_extern _puts
_puts:
	.quad ___wine$func$msvcr120$1755$puts
