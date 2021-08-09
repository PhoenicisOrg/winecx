	.text

	.align 3
	
	.globl __getch
	.private_extern __getch
__getch:
	.quad ___wine$func$msvcr120$706$_getch
