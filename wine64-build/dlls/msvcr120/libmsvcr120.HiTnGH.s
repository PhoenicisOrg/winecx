	.text

	.align 3
	
	.globl __fwrite_nolock
	.private_extern __fwrite_nolock
__fwrite_nolock:
	.quad ___wine$func$msvcr120$682$_fwrite_nolock
