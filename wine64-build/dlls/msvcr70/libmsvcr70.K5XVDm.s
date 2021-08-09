	.text

	.align 3
	
	.globl __wctime
	.private_extern __wctime
__wctime:
	.quad ___wine$func$msvcr70$511$_wctime
