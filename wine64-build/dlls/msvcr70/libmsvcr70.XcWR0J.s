	.text

	.align 3
	
	.globl _rand
	.private_extern _rand
_rand:
	.quad ___wine$func$msvcr70$695$rand
