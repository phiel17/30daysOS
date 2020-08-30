bits 32

GLOBAL	api_timerfree

api_timerfree:
	PUSH	EBX
	MOV		EDX, 19
	MOV		EBX, [ESP + 8]
	INT		0x40
	POP		EBX
	RET
