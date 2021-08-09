	.text

	.align 3
	
	.globl __sleep
	.private_extern __sleep
__sleep:
	.quad ___wine$func$msvcr70$427$_sleep
