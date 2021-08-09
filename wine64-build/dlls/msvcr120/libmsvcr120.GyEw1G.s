	.text

	.align 3
	
	.globl _setbuf
	.private_extern _setbuf
_setbuf:
	.quad ___wine$func$msvcr120$1787$setbuf
