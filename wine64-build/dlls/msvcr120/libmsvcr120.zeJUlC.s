	.text

	.align 3
	
	.globl __scwprintf
	.private_extern __scwprintf
__scwprintf:
	.quad ___wine$func$msvcr120$1076$_scwprintf
