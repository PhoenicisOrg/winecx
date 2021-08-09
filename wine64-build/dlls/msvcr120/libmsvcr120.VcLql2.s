	.text

	.align 3
	
	.globl __getch_nolock
	.private_extern __getch_nolock
__getch_nolock:
	.quad ___wine$func$msvcr120$707$_getch_nolock
