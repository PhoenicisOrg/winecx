	.text

	.align 3
	
	.globl __beep
	.private_extern __beep
__beep:
	.quad ___wine$func$msvcr120$517$_beep
