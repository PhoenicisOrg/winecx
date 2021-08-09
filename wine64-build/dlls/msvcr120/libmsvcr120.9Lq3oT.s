	.text

	.align 3
	
	.globl ___crtSetUnhandledExceptionFilter
	.private_extern ___crtSetUnhandledExceptionFilter
___crtSetUnhandledExceptionFilter:
	.quad ___wine$func$msvcr120$416$__crtSetUnhandledExceptionFilter
