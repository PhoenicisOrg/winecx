	.text

	.align 3
	
	.globl ___doserrno
	.private_extern ___doserrno
___doserrno:
	.quad ___wine$func$msvcr120$422$__doserrno
