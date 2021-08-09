	.text

	.align 3
	
	.globl __getmaxstdio
	.private_extern __getmaxstdio
__getmaxstdio:
	.quad ___wine$func$msvcr70$258$_getmaxstdio
