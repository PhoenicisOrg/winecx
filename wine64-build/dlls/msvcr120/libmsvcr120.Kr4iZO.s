	.text

	.align 3
	
	.globl _exit
	.private_extern _exit
_exit:
	.quad ___wine$func$msvcr120$1568$exit
