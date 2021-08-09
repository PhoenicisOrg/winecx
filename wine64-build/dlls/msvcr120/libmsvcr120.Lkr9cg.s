	.text

	.align 3
	
	.globl _perror
	.private_extern _perror
_perror:
	.quad ___wine$func$msvcr120$1748$perror
