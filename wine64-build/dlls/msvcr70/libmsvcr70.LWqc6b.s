	.text

	.align 3
	
	.globl __lock
	.private_extern __lock
__lock:
	.quad ___wine$func$msvcr70$319$_lock
