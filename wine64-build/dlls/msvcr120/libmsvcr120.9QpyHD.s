	.text

	.align 3
	
	.globl _setlocale
	.private_extern _setlocale
_setlocale:
	.quad ___wine$func$msvcr120$1789$setlocale
