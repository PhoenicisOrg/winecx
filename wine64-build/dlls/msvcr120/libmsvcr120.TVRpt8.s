	.text

	.align 3
	
	.globl __kbhit
	.private_extern __kbhit
__kbhit:
	.quad ___wine$func$msvcr120$855$_kbhit
