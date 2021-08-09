	.text

	.align 3
	
	.globl __strtime
	.private_extern __strtime
__strtime:
	.quad ___wine$func$msvcr70$459$_strtime
