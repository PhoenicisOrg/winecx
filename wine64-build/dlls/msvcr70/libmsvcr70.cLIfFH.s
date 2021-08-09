	.text

	.align 3
	
	.globl _signal
	.private_extern _signal
_signal:
	.quad ___wine$func$msvcr70$704$signal
