	.text

	.align 3
	
	.globl _fesetenv
	.private_extern _fesetenv
_fesetenv:
	.quad ___wine$func$msvcr120$1591$fesetenv
