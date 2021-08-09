	.text

	.align 3
	
	.globl __cwprintf
	.private_extern __cwprintf
__cwprintf:
	.quad ___wine$func$msvcr120$569$_cwprintf
