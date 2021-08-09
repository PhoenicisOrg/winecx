	.text

	.align 3
	
	.globl _tmpnam
	.private_extern _tmpnam
_tmpnam:
	.quad ___wine$func$msvcr70$739$tmpnam
