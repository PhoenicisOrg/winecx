	.text

	.align 3
	
	.globl _localtime
	.private_extern _localtime
_localtime:
	.quad ___wine$func$msvcr70$670$localtime
