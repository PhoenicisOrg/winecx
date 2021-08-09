	.text

	.align 3
	
	.globl __beep
	.private_extern __beep
__beep:
	.quad ___wine$func$msvcr70$162$_beep
