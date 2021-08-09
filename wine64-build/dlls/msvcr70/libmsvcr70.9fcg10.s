	.text

	.align 3
	
	.globl __mkdir
	.private_extern __mkdir
__mkdir:
	.quad ___wine$func$msvcr70$386$_mkdir
