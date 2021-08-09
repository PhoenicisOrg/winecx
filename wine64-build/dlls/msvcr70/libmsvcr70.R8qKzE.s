	.text

	.align 3
	
	.globl _time
	.private_extern _time
_time:
	.quad ___wine$func$msvcr70$737$time
