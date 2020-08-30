bits 32

GLOBAL	api_closewin

api_closewin:
	PUSH	EBX
	MOV		EDX, 14
	MOV		EBX, [ESP + 8]
	INT		0x40
	POP		EBX
	RET
