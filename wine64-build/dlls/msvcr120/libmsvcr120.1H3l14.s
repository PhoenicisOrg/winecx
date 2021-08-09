	.text

	.align 3
	
	.globl __fgetc_nolock
	.private_extern __fgetc_nolock
__fgetc_nolock:
	.quad ___wine$func$msvcr120$619$_fgetc_nolock
