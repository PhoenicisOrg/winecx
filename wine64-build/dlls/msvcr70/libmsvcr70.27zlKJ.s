	.text

	.align 3
	
	.globl _gmtime
	.private_extern _gmtime
_gmtime:
	.quad ___wine$func$msvcr70$639$gmtime
