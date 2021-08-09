	.text

	.align 3
	
	.globl __beginthread
	.private_extern __beginthread
__beginthread:
	.quad ___wine$func$msvcr70$163$_beginthread
