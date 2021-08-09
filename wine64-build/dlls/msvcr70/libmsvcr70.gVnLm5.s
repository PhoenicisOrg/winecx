	.text

	.align 3
	
	.globl ___doserrno
	.private_extern ___doserrno
___doserrno:
	.quad ___wine$func$msvcr70$87$__doserrno
