	.text

	.align 3
	
	.globl __kbhit
	.private_extern __kbhit
__kbhit:
	.quad ___wine$func$msvcr70$315$_kbhit
