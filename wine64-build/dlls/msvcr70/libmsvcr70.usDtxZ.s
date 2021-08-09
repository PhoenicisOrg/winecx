	.text

	.align 3
	
	.globl __vscprintf
	.private_extern __vscprintf
__vscprintf:
	.quad ___wine$func$msvcr70$487$_vscprintf
