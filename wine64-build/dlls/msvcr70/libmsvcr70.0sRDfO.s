	.text

	.align 3
	
	.globl __tzset
	.private_extern __tzset
__tzset:
	.quad ___wine$func$msvcr70$474$_tzset