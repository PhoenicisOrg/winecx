	.text

	.align 3
	
	.globl _vfwprintf
	.private_extern _vfwprintf
_vfwprintf:
	.quad ___wine$func$msvcr120$1866$vfwprintf
