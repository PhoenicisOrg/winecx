	.text

	.align 3
	
	.globl __cprintf
	.private_extern __cprintf
__cprintf:
	.quad ___wine$func$msvcr70$183$_cprintf
