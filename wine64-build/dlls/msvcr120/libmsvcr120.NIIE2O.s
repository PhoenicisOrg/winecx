	.text

	.align 3
	
	.globl ___crt_debugger_hook
	.private_extern ___crt_debugger_hook
___crt_debugger_hook:
	.quad ___wine$func$msvcr120$559$__crt_debugger_hook
