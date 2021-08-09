	.text

	.align 3
	
	.globl _sprintf
	.private_extern _sprintf
_sprintf:
	.quad ___wine$func$msvcr120$1796$sprintf
