	.text

	.align 3
	
	.globl _asctime
	.private_extern _asctime
_asctime:
	.quad ___wine$func$msvcr70$584$asctime
