	.text

	.align 3
	
	.globl _localeconv
	.private_extern _localeconv
_localeconv:
	.quad ___wine$func$msvcr70$669$localeconv
