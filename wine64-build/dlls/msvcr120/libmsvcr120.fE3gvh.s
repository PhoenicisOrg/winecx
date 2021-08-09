	.text

	.align 3
	
	.globl __get_tzname
	.private_extern __get_tzname
__get_tzname:
	.quad ___wine$func$msvcr120$702$_get_tzname
