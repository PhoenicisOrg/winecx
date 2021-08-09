	.text

	.align 3
	
	.globl _rand_s
	.private_extern _rand_s
_rand_s:
	.quad ___wine$func$msvcr120$1762$rand_s
