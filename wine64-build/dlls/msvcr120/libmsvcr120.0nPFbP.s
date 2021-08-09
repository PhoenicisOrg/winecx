	.text

	.align 3
	
	.globl __ungetc_nolock
	.private_extern __ungetc_nolock
__ungetc_nolock:
	.quad ___wine$func$msvcr120$1224$_ungetc_nolock
