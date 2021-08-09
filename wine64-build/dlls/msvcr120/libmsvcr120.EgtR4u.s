	.text

	.align 3
	
	.globl __snprintf
	.private_extern __snprintf
__snprintf:
	.quad ___wine$func$msvcr120$1102$_snprintf
