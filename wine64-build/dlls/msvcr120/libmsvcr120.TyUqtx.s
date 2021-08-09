	.text

	.align 3
	
	.globl _tmpnam_s
	.private_extern _tmpnam_s
_tmpnam_s:
	.quad ___wine$func$msvcr120$1851$tmpnam_s
