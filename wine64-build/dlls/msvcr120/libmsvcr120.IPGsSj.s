	.text

	.align 3
	
	.globl __putc_nolock
	.private_extern __putc_nolock
__putc_nolock:
	.quad ___wine$func$msvcr120$1047$_putc_nolock
