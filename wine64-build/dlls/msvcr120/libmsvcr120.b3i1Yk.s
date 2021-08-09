	.text

	.align 3
	
	.globl __getc_nolock
	.private_extern __getc_nolock
__getc_nolock:
	.quad ___wine$func$msvcr120$705$_getc_nolock
