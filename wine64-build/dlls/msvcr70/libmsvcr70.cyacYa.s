	.text

	.align 3
	
	.globl _strftime
	.private_extern _strftime
_strftime:
	.quad ___wine$func$msvcr70$718$strftime
