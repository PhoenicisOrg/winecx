	.text

	.align 3
	
	.globl __execve
	.private_extern __execve
__execve:
	.quad ___wine$func$msvcr70$208$_execve
