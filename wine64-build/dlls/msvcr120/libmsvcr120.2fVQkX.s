	.text

	.align 3
	
	.globl _nearbyint
	.private_extern _nearbyint
_nearbyint:
	.quad ___wine$func$msvcr120$1736$nearbyint
