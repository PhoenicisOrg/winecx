	.text

	.align 3
	
	.globl __getmaxstdio
	.private_extern __getmaxstdio
__getmaxstdio:
	.quad ___wine$func$msvcr120$716$_getmaxstdio
