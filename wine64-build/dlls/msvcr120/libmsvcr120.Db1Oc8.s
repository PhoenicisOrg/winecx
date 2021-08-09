	.text

	.align 3
	
	.globl __getwch_nolock
	.private_extern __getwch_nolock
__getwch_nolock:
	.quad ___wine$func$msvcr120$724$_getwch_nolock
