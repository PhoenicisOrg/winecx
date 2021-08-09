	.text

	.align 3
	
	.globl __scwprintf
	.private_extern __scwprintf
__scwprintf:
	.quad ___wine$func$msvcr70$415$_scwprintf
