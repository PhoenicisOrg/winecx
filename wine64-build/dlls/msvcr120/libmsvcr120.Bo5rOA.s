	.text

	.align 3
	
	.globl __beginthread
	.private_extern __beginthread
__beginthread:
	.quad ___wine$func$msvcr120$518$_beginthread
