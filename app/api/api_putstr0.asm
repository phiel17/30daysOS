bits 32

GLOBAL	api_putstr0

api_putstr0:
	PUSH	EBX
	MOV		EDX, 2
	MOV		EBX, [ESP + 8]
	INT		0x40
	POP		EBX
	RET
