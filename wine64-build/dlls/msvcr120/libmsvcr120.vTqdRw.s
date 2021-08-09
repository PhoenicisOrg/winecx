	.text

	.align 3
	
	.globl __getwche_nolock
	.private_extern __getwche_nolock
__getwche_nolock:
	.quad ___wine$func$msvcr120$726$_getwche_nolock
