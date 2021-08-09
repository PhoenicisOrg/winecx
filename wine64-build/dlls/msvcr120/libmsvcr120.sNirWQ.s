	.text

	.align 3
	
	.globl __fpreset
	.private_extern __fpreset
__fpreset:
	.quad ___wine$func$msvcr120$643$_fpreset
