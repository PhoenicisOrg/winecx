	.text

	.align 3
	
	.globl _mktime
	.private_extern _mktime
_mktime:
	.quad ___wine$func$msvcr70$683$mktime
