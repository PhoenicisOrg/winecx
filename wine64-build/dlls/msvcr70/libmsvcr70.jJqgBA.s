	.text

	.align 3
	
	.globl __scprintf
	.private_extern __scprintf
__scprintf:
	.quad ___wine$func$msvcr70$414$_scprintf
