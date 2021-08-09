	.text

	.align 3
	
	.globl __fclose_nolock
	.private_extern __fclose_nolock
__fclose_nolock:
	.quad ___wine$func$msvcr120$609$_fclose_nolock
