	.text

	.align 3
	
	.globl _wcspbrk
	.private_extern _wcspbrk
_wcspbrk:
	.quad ___wine$func$msvcr120$1903$wcspbrk
