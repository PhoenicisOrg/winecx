	.text

	.align 3
	
	.globl __mkdir
	.private_extern __mkdir
__mkdir:
	.quad ___wine$func$msvcr120$1024$_mkdir
