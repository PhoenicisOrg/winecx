	.text

	.align 3
	
	.globl __beginthreadex
	.private_extern __beginthreadex
__beginthreadex:
	.quad ___wine$func$msvcr120$519$_beginthreadex
