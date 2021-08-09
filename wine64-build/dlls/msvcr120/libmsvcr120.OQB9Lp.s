	.text

	.align 3
	
	.globl __putwch_nolock
	.private_extern __putwch_nolock
__putwch_nolock:
	.quad ___wine$func$msvcr120$1055$_putwch_nolock
