	.text

	.align 3
	
	.globl __isnan
	.private_extern __isnan
__isnan:
	.quad ___wine$func$msvcr70$309$_isnan
