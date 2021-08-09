	.text

	.align 3
	
	.globl ___sys_errlist
	.private_extern ___sys_errlist
___sys_errlist:
	.quad ___wine$func$msvcr120$472$__sys_errlist
