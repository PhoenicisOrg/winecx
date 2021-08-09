	.text

	.align 3
	
	.globl _lrintf
	.private_extern _lrintf
_lrintf:
	.quad ___wine$func$msvcr120$1710$lrintf
