	.text

	.align 3
	
	.globl __set_security_error_handler
	.private_extern __set_security_error_handler
__set_security_error_handler:
	.quad ___wine$func$msvcr70$420$_set_security_error_handler
