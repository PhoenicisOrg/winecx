	.text

	.align 3
	
	.globl __putch_nolock
	.private_extern __putch_nolock
__putch_nolock:
	.quad ___wine$func$msvcr120$1049$_putch_nolock
