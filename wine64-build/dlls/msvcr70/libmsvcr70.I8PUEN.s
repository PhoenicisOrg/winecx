	.text

	.align 3
	
	.globl __exit
	.private_extern __exit
__exit:
	.quad ___wine$func$msvcr70$211$_exit
