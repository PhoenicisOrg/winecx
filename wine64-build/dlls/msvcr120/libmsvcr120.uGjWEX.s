	.text

	.align 3
	
	.globl __execve
	.private_extern __execve
__execve:
	.quad ___wine$func$msvcr120$604$_execve
