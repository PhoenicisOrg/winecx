	.text

	.align 3
	
	.globl __vsnprintf
	.private_extern __vsnprintf
__vsnprintf:
	.quad ___wine$func$msvcr70$489$_vsnprintf
