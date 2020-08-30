bits 32

GLOBAL api_getkey

api_getkey:
	MOV		EDX, 15
	MOV		EAX, [ESP + 4]	; mode
	INT		0x40
	RET
