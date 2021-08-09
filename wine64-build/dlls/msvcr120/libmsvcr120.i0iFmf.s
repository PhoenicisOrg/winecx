	.text

	.align 3
	
	.globl _fegetenv
	.private_extern _fegetenv
_fegetenv:
	.quad ___wine$func$msvcr120$1584$fegetenv
