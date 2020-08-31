bits 32

GLOBAL	api_getlang

api_getlang:
	MOV		EDX, 27
	INT		0x40
	RET
