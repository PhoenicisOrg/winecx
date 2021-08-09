	.text

	.align 3
	
	.globl __vcprintf
	.private_extern __vcprintf
__vcprintf:
	.quad ___wine$func$msvcr120$1237$_vcprintf
