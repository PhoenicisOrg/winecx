	.text

	.align 3
	
	.globl _rint
	.private_extern _rint
_rint:
	.quad ___wine$func$msvcr120$1773$rint
