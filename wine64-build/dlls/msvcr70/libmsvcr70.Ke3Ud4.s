	.text

	.align 3
	
	.globl _fclose
	.private_extern _fclose
_fclose:
	.quad ___wine$func$msvcr70$605$fclose
