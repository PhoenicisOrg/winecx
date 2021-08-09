	.text

	.align 3
	
	.globl __ftell_nolock
	.private_extern __ftell_nolock
__ftell_nolock:
	.quad ___wine$func$msvcr120$668$_ftell_nolock
