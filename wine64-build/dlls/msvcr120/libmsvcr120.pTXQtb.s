	.text

	.align 3
	
	.globl ___crtSleep
	.private_extern ___crtSleep
___crtSleep:
	.quad ___wine$func$msvcr120$418$__crtSleep
