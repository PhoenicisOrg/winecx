	.text

	.align 3
	
	.globl __fseek_nolock
	.private_extern __fseek_nolock
__fseek_nolock:
	.quad ___wine$func$msvcr120$660$_fseek_nolock
