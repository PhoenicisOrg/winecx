	.text

	.align 3
	
	.globl __getwc_nolock
	.private_extern __getwc_nolock
__getwc_nolock:
	.quad ___wine$func$msvcr120$722$_getwc_nolock
