	.text

	.align 3
	
	.globl _signal
	.private_extern _signal
_signal:
	.quad ___wine$func$msvcr120$1791$signal
