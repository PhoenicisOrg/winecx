	.text

	.align 3
	
	.globl _exit
	.private_extern _exit
_exit:
	.quad ___wine$func$msvcr70$602$exit
