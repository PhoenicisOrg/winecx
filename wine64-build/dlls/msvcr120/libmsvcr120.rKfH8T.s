	.text

	.align 3
	
	.globl __execlp
	.private_extern __execlp
__execlp:
	.quad ___wine$func$msvcr120$601$_execlp
