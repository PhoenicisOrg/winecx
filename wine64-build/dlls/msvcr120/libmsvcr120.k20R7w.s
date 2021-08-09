	.text

	.align 3
	
	.globl __fflush_nolock
	.private_extern __fflush_nolock
__fflush_nolock:
	.quad ___wine$func$msvcr120$618$_fflush_nolock
