	.text

	.align 3
	
	.globl _getwchar
	.private_extern _getwchar
_getwchar:
	.quad ___wine$func$msvcr70$638$getwchar
