	.text

	.align 3
	
	.globl ___dllonexit
	.private_extern ___dllonexit
___dllonexit:
	.quad ___wine$func$msvcr70$86$__dllonexit
