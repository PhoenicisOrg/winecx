	.text

	.align 3
	
	.globl _vfprintf
	.private_extern _vfprintf
_vfprintf:
	.quad ___wine$func$msvcr120$1862$vfprintf
