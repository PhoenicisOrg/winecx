	.text

	.align 3
	
	.globl __unlock
	.private_extern __unlock
__unlock:
	.quad ___wine$func$msvcr70$484$_unlock
