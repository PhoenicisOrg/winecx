	.text

	.align 3
	
	.globl __ftime
	.private_extern __ftime
__ftime:
	.quad ___wine$func$msvcr70$242$_ftime
