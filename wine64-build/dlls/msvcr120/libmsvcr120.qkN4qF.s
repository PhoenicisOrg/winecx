	.text

	.align 3
	
	.globl _wcslen
	.private_extern _wcslen
_wcslen:
	.quad ___wine$func$msvcr120$1896$wcslen
