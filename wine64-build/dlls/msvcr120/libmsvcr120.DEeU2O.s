	.text

	.align 3
	
	.globl __heapchk
	.private_extern __heapchk
__heapchk:
	.quad ___wine$func$msvcr120$734$_heapchk
