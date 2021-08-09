	.text

	.align 3
	
	.globl ___security_error_handler
	.private_extern ___security_error_handler
___security_error_handler:
	.quad ___wine$func$msvcr70$133$__security_error_handler
