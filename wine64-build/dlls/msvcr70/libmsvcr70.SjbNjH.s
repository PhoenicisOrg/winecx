	.text

	.align 3
	
	.globl _fopen
	.private_extern _fopen
_fopen:
	.quad ___wine$func$msvcr70$616$fopen
