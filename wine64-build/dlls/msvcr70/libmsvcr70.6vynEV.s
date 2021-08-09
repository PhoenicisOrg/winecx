	.text

	.align 3
	
	.globl __fcloseall
	.private_extern __fcloseall
__fcloseall:
	.quad ___wine$func$msvcr70$213$_fcloseall
