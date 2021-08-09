	.text

	.align 3
	
	.globl __fcloseall
	.private_extern __fcloseall
__fcloseall:
	.quad ___wine$func$msvcr120$610$_fcloseall
