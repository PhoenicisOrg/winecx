	.text

	.align 3
	
	.globl _vswprintf
	.private_extern _vswprintf
_vswprintf:
	.quad ___wine$func$msvcr70$750$vswprintf
