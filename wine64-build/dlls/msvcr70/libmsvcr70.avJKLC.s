	.text

	.align 3
	
	.globl __get_osfhandle
	.private_extern __get_osfhandle
__get_osfhandle:
	.quad ___wine$func$msvcr70$248$_get_osfhandle
