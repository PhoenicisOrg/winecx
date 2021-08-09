	.text

	.align 3
	
	.globl _fclose
	.private_extern _fclose
_fclose:
	.quad ___wine$func$msvcr120$1579$fclose
