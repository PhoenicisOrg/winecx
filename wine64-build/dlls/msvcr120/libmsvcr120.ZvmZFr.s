	.text

	.align 3
	
	.globl __vswprintf
	.private_extern __vswprintf
__vswprintf:
	.quad ___wine$func$msvcr120$1283$_vswprintf
