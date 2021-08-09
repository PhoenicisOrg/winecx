	.text

	.align 3
	
	.globl _wprintf
	.private_extern _wprintf
_wprintf:
	.quad ___wine$func$msvcr70$774$wprintf
