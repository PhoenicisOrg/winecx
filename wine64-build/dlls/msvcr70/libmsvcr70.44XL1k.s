	.text

	.align 3
	
	.globl _clearerr
	.private_extern _clearerr
_clearerr:
	.quad ___wine$func$msvcr70$595$clearerr
