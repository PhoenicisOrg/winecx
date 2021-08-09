	.text

	.align 3
	
	.globl __fpreset
	.private_extern __fpreset
__fpreset:
	.quad ___wine$func$msvcr70$235$_fpreset
