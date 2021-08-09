	.text

	.align 3
	
	.globl ___crtTerminateProcess
	.private_extern ___crtTerminateProcess
___crtTerminateProcess:
	.quad ___wine$func$msvcr120$417$__crtTerminateProcess
