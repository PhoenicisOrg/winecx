	.text

	.align 3
	
	.globl __vscwprintf
	.private_extern __vscwprintf
__vscwprintf:
	.quad ___wine$func$msvcr70$488$_vscwprintf
