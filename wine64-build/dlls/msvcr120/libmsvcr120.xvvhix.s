	.text

	.align 3
	
	.globl _wcsftime
	.private_extern _wcsftime
_wcsftime:
	.quad ___wine$func$msvcr120$1895$wcsftime
