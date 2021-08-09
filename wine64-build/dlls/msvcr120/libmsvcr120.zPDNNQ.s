	.text

	.align 3
	
	.globl __getche_nolock
	.private_extern __getche_nolock
__getche_nolock:
	.quad ___wine$func$msvcr120$709$_getche_nolock
