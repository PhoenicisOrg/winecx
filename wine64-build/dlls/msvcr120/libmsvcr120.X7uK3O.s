	.text

	.align 3
	
	.globl __abnormal_termination
	.private_extern __abnormal_termination
__abnormal_termination:
	.quad ___wine$func$msvcr120$490$_abnormal_termination
