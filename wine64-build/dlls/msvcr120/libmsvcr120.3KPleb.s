	.text

	.align 3
	
	.globl _tmpfile
	.private_extern _tmpfile
_tmpfile:
	.quad ___wine$func$msvcr120$1848$tmpfile
