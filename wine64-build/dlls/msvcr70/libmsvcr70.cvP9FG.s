	.text

	.align 3
	
	.globl __stricmp
	.private_extern __stricmp
__stricmp:
	.quad ___wine$func$msvcr70$450$_stricmp
