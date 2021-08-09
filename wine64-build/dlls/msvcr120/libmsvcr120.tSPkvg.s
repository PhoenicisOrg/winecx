	.text

	.align 3
	
	.globl _rintf
	.private_extern _rintf
_rintf:
	.quad ___wine$func$msvcr120$1774$rintf
