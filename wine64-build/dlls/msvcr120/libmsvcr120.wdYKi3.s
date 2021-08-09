	.text

	.align 3
	
	.globl __scprintf
	.private_extern __scprintf
__scprintf:
	.quad ___wine$func$msvcr120$1072$_scprintf
