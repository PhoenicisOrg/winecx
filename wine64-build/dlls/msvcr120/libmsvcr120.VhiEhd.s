	.text

	.align 3
	
	.globl __fread_nolock
	.private_extern __fread_nolock
__fread_nolock:
	.quad ___wine$func$msvcr120$652$_fread_nolock
