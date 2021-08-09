	.text

	.align 3
	
	.globl __vscprintf
	.private_extern __vscprintf
__vscprintf:
	.quad ___wine$func$msvcr120$1261$_vscprintf
