	.text

	.align 3
	
	.globl __putwc_nolock
	.private_extern __putwc_nolock
__putwc_nolock:
	.quad ___wine$func$msvcr120$1053$_putwc_nolock
