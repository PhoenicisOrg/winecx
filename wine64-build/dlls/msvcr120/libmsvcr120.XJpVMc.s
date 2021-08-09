	.text

	.align 3
	
	.globl __vsnprintf
	.private_extern __vsnprintf
__vsnprintf:
	.quad ___wine$func$msvcr120$1269$_vsnprintf
