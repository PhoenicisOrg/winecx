	.text

	.align 3
	
	.globl __vcwprintf
	.private_extern __vcwprintf
__vcwprintf:
	.quad ___wine$func$msvcr120$1243$_vcwprintf
