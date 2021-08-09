	.text

	.align 3
	
	.globl _ctime
	.private_extern _ctime
_ctime:
	.quad ___wine$func$msvcr70$599$ctime
